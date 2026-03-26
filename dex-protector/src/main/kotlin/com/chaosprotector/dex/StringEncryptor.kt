package com.chaosprotector.dex

import org.objectweb.asm.*
import org.objectweb.asm.tree.*

/**
 * DEX String Encryption
 *
 * Encrypts string constants in Java/Kotlin bytecode.
 * Each string is replaced with a call to a decryptor method
 * that decrypts at runtime using XOR + position cipher.
 *
 * Before: ldc "secret_api_key"
 * After:  invokestatic DecryptorClass.d("encrypted_bytes", key)
 */
class StringEncryptor(private val key: Int = 0x5A3C_9E7F.toInt()) {

    fun transform(classNode: ClassNode): Boolean {
        var changed = false

        // Skip the decryptor class itself
        if (classNode.name.endsWith("\$ChaosDecryptor"))
            return false

        for (method in classNode.methods) {
            val insns = method.instructions ?: continue
            val iter = insns.iterator()

            while (iter.hasNext()) {
                val insn = iter.next()
                if (insn is LdcInsnNode && insn.cst is String) {
                    val original = insn.cst as String
                    if (original.length < 3) continue // Skip very short strings

                    val encrypted = encrypt(original)

                    // Replace: ldc "string" -> invokestatic ChaosDecryptor.d(encBytes, key)
                    val newInsns = InsnList()
                    newInsns.add(LdcInsnNode(encrypted))
                    newInsns.add(LdcInsnNode(key))
                    newInsns.add(MethodInsnNode(
                        Opcodes.INVOKESTATIC,
                        classNode.name + "\$ChaosDecryptor",
                        "d",
                        "(Ljava/lang/String;I)Ljava/lang/String;",
                        false
                    ))

                    insns.insert(insn, newInsns)
                    insns.remove(insn)
                    changed = true
                }
            }
        }

        if (changed) {
            // Inject decryptor inner class
            injectDecryptor(classNode)
        }

        return changed
    }

    private fun encrypt(s: String): String {
        val bytes = s.toByteArray(Charsets.UTF_8)
        val result = ByteArray(bytes.size)
        for (i in bytes.indices) {
            result[i] = (bytes[i].toInt() xor ((key shr (i % 4 * 8)) and 0xFF) xor (i * 7 + 13)).toByte()
        }
        return java.util.Base64.getEncoder().encodeToString(result)
    }

    private fun injectDecryptor(classNode: ClassNode) {
        // Add a static inner class with the decryptor method
        // The decryptor is: decode base64, XOR with key+position
        classNode.innerClasses.add(InnerClassNode(
            classNode.name + "\$ChaosDecryptor",
            classNode.name,
            "ChaosDecryptor",
            Opcodes.ACC_PRIVATE or Opcodes.ACC_STATIC
        ))
    }

    companion object {
        fun createDecryptorClass(outerClassName: String, key: Int): ByteArray {
            val cw = ClassWriter(ClassWriter.COMPUTE_FRAMES or ClassWriter.COMPUTE_MAXS)
            val className = "$outerClassName\$ChaosDecryptor"

            cw.visit(Opcodes.V11, Opcodes.ACC_PUBLIC or Opcodes.ACC_STATIC,
                className, null, "java/lang/Object", null)

            // static String d(String encoded, int key)
            val mv = cw.visitMethod(
                Opcodes.ACC_PUBLIC or Opcodes.ACC_STATIC,
                "d",
                "(Ljava/lang/String;I)Ljava/lang/String;",
                null, null
            )
            mv.visitCode()

            // byte[] data = Base64.getDecoder().decode(encoded);
            mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/util/Base64", "getDecoder",
                "()Ljava/util/Base64\$Decoder;", false)
            mv.visitVarInsn(Opcodes.ALOAD, 0)
            mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, "java/util/Base64\$Decoder", "decode",
                "(Ljava/lang/String;)[B", false)
            mv.visitVarInsn(Opcodes.ASTORE, 2) // data

            // for (int i = 0; i < data.length; i++) data[i] ^= ((key >> (i%4*8)) & 0xFF) ^ (i*7+13);
            mv.visitInsn(Opcodes.ICONST_0)
            mv.visitVarInsn(Opcodes.ISTORE, 3) // i = 0

            val loopStart = Label()
            val loopEnd = Label()
            mv.visitLabel(loopStart)
            mv.visitVarInsn(Opcodes.ILOAD, 3)
            mv.visitVarInsn(Opcodes.ALOAD, 2)
            mv.visitInsn(Opcodes.ARRAYLENGTH)
            mv.visitJumpInsn(Opcodes.IF_ICMPGE, loopEnd)

            // data[i] ^= ((key >> ((i%4)*8)) & 0xFF) ^ (i*7+13)
            mv.visitVarInsn(Opcodes.ALOAD, 2)
            mv.visitVarInsn(Opcodes.ILOAD, 3)
            mv.visitVarInsn(Opcodes.ALOAD, 2)
            mv.visitVarInsn(Opcodes.ILOAD, 3)
            mv.visitInsn(Opcodes.BALOAD)

            // (key >> ((i%4)*8)) & 0xFF
            mv.visitVarInsn(Opcodes.ILOAD, 1)
            mv.visitVarInsn(Opcodes.ILOAD, 3)
            mv.visitInsn(Opcodes.ICONST_4)
            mv.visitInsn(Opcodes.IREM)
            mv.visitIntInsn(Opcodes.BIPUSH, 8)
            mv.visitInsn(Opcodes.IMUL)
            mv.visitInsn(Opcodes.ISHR)
            mv.visitIntInsn(Opcodes.SIPUSH, 0xFF)
            mv.visitInsn(Opcodes.IAND)
            mv.visitInsn(Opcodes.IXOR)

            // ^ (i*7+13)
            mv.visitVarInsn(Opcodes.ILOAD, 3)
            mv.visitIntInsn(Opcodes.BIPUSH, 7)
            mv.visitInsn(Opcodes.IMUL)
            mv.visitIntInsn(Opcodes.BIPUSH, 13)
            mv.visitInsn(Opcodes.IADD)
            mv.visitInsn(Opcodes.IXOR)

            mv.visitInsn(Opcodes.I2B)
            mv.visitInsn(Opcodes.BASTORE)

            mv.visitIincInsn(3, 1) // i++
            mv.visitJumpInsn(Opcodes.GOTO, loopStart)
            mv.visitLabel(loopEnd)

            // return new String(data, "UTF-8");
            mv.visitTypeInsn(Opcodes.NEW, "java/lang/String")
            mv.visitInsn(Opcodes.DUP)
            mv.visitVarInsn(Opcodes.ALOAD, 2)
            mv.visitLdcInsn("UTF-8")
            mv.visitMethodInsn(Opcodes.INVOKESPECIAL, "java/lang/String", "<init>",
                "([BLjava/lang/String;)V", false)
            mv.visitInsn(Opcodes.ARETURN)

            mv.visitMaxs(6, 4)
            mv.visitEnd()
            cw.visitEnd()

            return cw.toByteArray()
        }
    }
}
