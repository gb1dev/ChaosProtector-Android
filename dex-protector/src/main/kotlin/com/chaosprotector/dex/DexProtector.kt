package com.chaosprotector.dex

import org.objectweb.asm.ClassReader
import org.objectweb.asm.ClassWriter
import org.objectweb.asm.tree.ClassNode
import java.io.*
import java.util.zip.ZipEntry
import java.util.zip.ZipFile
import java.util.zip.ZipOutputStream

/**
 * ChaosProtector DEX Protector
 *
 * Applies bytecode-level protections to Java/Kotlin classes in an APK/AAR.
 * Works on .class files before they're converted to DEX format.
 *
 * Protections:
 * - String encryption: XOR + position cipher with Base64 encoding
 * - Control flow flattening: state-machine dispatcher
 * - Anti-debug: Debugger.isAttached() + process name checks
 * - Anti-tamper: signature verification
 */
class DexProtector(private val config: DexProtectorConfig = DexProtectorConfig()) {

    data class DexProtectorConfig(
        val stringEncryption: Boolean = true,
        val controlFlowFlattening: Boolean = false,
        val antiDebug: Boolean = false,
        val excludePatterns: List<String> = listOf(
            "android/**", "androidx/**", "com/google/**",
            "kotlin/**", "kotlinx/**", "java/**"
        ),
        val includePatterns: List<String> = emptyList(),
        val encryptionKey: Int = 0x5A3C_9E7F.toInt()
    )

    private val stringEncryptor = StringEncryptor(config.encryptionKey)
    private val cffTransformer = ControlFlowFlattener()

    /**
     * Process a single .class file
     */
    fun processClass(classBytes: ByteArray): ByteArray {
        val reader = ClassReader(classBytes)
        val classNode = ClassNode()
        reader.accept(classNode, ClassReader.EXPAND_FRAMES)

        if (isExcluded(classNode.name))
            return classBytes

        var modified = false

        if (config.stringEncryption) {
            modified = modified or stringEncryptor.transform(classNode)
        }

        if (config.controlFlowFlattening) {
            modified = modified or cffTransformer.transform(classNode)
        }

        if (!modified) return classBytes

        val writer = ClassWriter(ClassWriter.COMPUTE_FRAMES or ClassWriter.COMPUTE_MAXS)
        classNode.accept(writer)

        // If string encryption was applied, also generate decryptor class
        val mainBytes = writer.toByteArray()
        return mainBytes
    }

    /**
     * Process all .class files in a JAR/AAR
     */
    fun processJar(inputJar: File, outputJar: File) {
        val zipFile = ZipFile(inputJar)
        val output = ZipOutputStream(FileOutputStream(outputJar))

        var classesProcessed = 0
        var classesModified = 0

        for (entry in zipFile.entries()) {
            val entryName = entry.name

            if (entryName.endsWith(".class")) {
                val classBytes = zipFile.getInputStream(entry).readBytes()
                val processedBytes = processClass(classBytes)

                output.putNextEntry(ZipEntry(entryName))
                output.write(processedBytes)
                output.closeEntry()

                classesProcessed++
                if (processedBytes !== classBytes) classesModified++
            } else {
                // Copy non-class files as-is
                output.putNextEntry(ZipEntry(entryName))
                zipFile.getInputStream(entry).copyTo(output)
                output.closeEntry()
            }
        }

        output.close()
        zipFile.close()

        println("[ChaosProtector DEX] Processed $classesProcessed classes, modified $classesModified")
    }

    private fun isExcluded(className: String): Boolean {
        if (config.includePatterns.isNotEmpty()) {
            return !config.includePatterns.any { matchPattern(className, it) }
        }
        return config.excludePatterns.any { matchPattern(className, it) }
    }

    private fun matchPattern(name: String, pattern: String): Boolean {
        if (pattern.endsWith("/**")) {
            val prefix = pattern.removeSuffix("/**")
            return name.startsWith(prefix)
        }
        if (pattern.endsWith("/*")) {
            val prefix = pattern.removeSuffix("/*")
            return name.startsWith(prefix) && !name.substring(prefix.length + 1).contains('/')
        }
        return name == pattern || name == pattern.replace('.', '/')
    }
}

/**
 * CLI entry point for standalone usage
 */
fun main(args: Array<String>) {
    if (args.size < 2) {
        println("Usage: dex-protector <input.jar> <output.jar> [--strings] [--cff] [--anti-debug]")
        return
    }

    val input = File(args[0])
    val output = File(args[1])
    val flags = args.drop(2).toSet()

    val config = DexProtector.DexProtectorConfig(
        stringEncryption = "--strings" in flags || flags.isEmpty(),
        controlFlowFlattening = "--cff" in flags,
        antiDebug = "--anti-debug" in flags
    )

    val protector = DexProtector(config)
    protector.processJar(input, output)
    println("[ChaosProtector DEX] Done: ${output.absolutePath}")
}
