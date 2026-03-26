package com.chaosprotector.dex

import org.objectweb.asm.Opcodes
import org.objectweb.asm.tree.*

/**
 * DEX Control Flow Flattening
 *
 * Transforms method control flow from structured if/else/loops
 * into a state-machine dispatcher:
 *
 * Before:
 *   if (x > 0) { doA(); } else { doB(); }
 *   doC();
 *
 * After:
 *   int state = 0;
 *   while (true) {
 *     switch (state) {
 *       case 0: if (x > 0) state = 1; else state = 2; break;
 *       case 1: doA(); state = 3; break;
 *       case 2: doB(); state = 3; break;
 *       case 3: doC(); state = -1; break;
 *       default: return;
 *     }
 *   }
 */
class ControlFlowFlattener {

    fun transform(classNode: ClassNode): Boolean {
        var changed = false

        for (method in classNode.methods) {
            if (method.instructions == null || method.instructions.size() < 10)
                continue
            if (method.name == "<init>" || method.name == "<clinit>")
                continue
            if ((method.access and Opcodes.ACC_ABSTRACT) != 0)
                continue
            if ((method.access and Opcodes.ACC_NATIVE) != 0)
                continue

            if (flattenMethod(method)) {
                changed = true
            }
        }

        return changed
    }

    private fun flattenMethod(method: MethodNode): Boolean {
        // Split method into basic blocks
        val blocks = splitIntoBlocks(method)
        if (blocks.size < 3) return false // Not worth flattening

        val insns = method.instructions
        val newInsns = InsnList()

        // State variable at first available local slot
        val stateVar = method.maxLocals
        method.maxLocals += 1

        // Initialize state = 0
        newInsns.add(InsnNode(Opcodes.ICONST_0))
        newInsns.add(VarInsnNode(Opcodes.ISTORE, stateVar))

        // Dispatcher loop
        val loopLabel = LabelNode()
        val defaultLabel = LabelNode()
        val endLabel = LabelNode()

        newInsns.add(loopLabel)
        newInsns.add(VarInsnNode(Opcodes.ILOAD, stateVar))

        // Create labels for each block
        val blockLabels = blocks.mapIndexed { idx, _ -> LabelNode() to idx }
        val lookupLabels = blockLabels.map { (label, idx) -> idx to label }.toTypedArray()

        // TableSwitch (0 to blocks.size-1)
        val labels = blockLabels.map { it.first }.toTypedArray()
        newInsns.add(TableSwitchInsnNode(0, blocks.size - 1, defaultLabel, *labels))

        // Emit each block with state transition
        for ((idx, block) in blocks.withIndex()) {
            newInsns.add(blockLabels[idx].first)

            // Copy block instructions (except terminal)
            for (insnIdx in block.indices) {
                val insn = block[insnIdx]
                if (insnIdx == block.lastIndex && isTerminalInsn(insn)) {
                    // Replace terminal with state assignment
                    val nextState = if (idx + 1 < blocks.size) idx + 1 else -1
                    newInsns.add(intConstInsn(nextState))
                    newInsns.add(VarInsnNode(Opcodes.ISTORE, stateVar))
                    newInsns.add(JumpInsnNode(Opcodes.GOTO, loopLabel))
                } else {
                    newInsns.add(insn.clone(mapOf()))
                }
            }
        }

        // Default: return
        newInsns.add(defaultLabel)
        when {
            method.desc.endsWith(")V") -> newInsns.add(InsnNode(Opcodes.RETURN))
            method.desc.endsWith(")I") || method.desc.endsWith(")Z") -> {
                newInsns.add(InsnNode(Opcodes.ICONST_0))
                newInsns.add(InsnNode(Opcodes.IRETURN))
            }
            method.desc.endsWith(")J") -> {
                newInsns.add(InsnNode(Opcodes.LCONST_0))
                newInsns.add(InsnNode(Opcodes.LRETURN))
            }
            else -> {
                newInsns.add(InsnNode(Opcodes.ACONST_NULL))
                newInsns.add(InsnNode(Opcodes.ARETURN))
            }
        }

        method.instructions = newInsns
        return true
    }

    private fun splitIntoBlocks(method: MethodNode): List<List<AbstractInsnNode>> {
        val blocks = mutableListOf<MutableList<AbstractInsnNode>>()
        var currentBlock = mutableListOf<AbstractInsnNode>()

        for (insn in method.instructions) {
            if (insn is LabelNode || insn is LineNumberNode || insn is FrameNode)
                continue

            currentBlock.add(insn)

            if (isTerminalInsn(insn)) {
                if (currentBlock.isNotEmpty()) {
                    blocks.add(currentBlock)
                    currentBlock = mutableListOf()
                }
            }
        }

        if (currentBlock.isNotEmpty()) {
            blocks.add(currentBlock)
        }

        return blocks
    }

    private fun isTerminalInsn(insn: AbstractInsnNode): Boolean {
        return when (insn.opcode) {
            Opcodes.GOTO, Opcodes.IRETURN, Opcodes.LRETURN, Opcodes.FRETURN,
            Opcodes.DRETURN, Opcodes.ARETURN, Opcodes.RETURN,
            Opcodes.TABLESWITCH, Opcodes.LOOKUPSWITCH,
            Opcodes.IF_ICMPEQ, Opcodes.IF_ICMPNE, Opcodes.IF_ICMPLT,
            Opcodes.IF_ICMPGE, Opcodes.IF_ICMPGT, Opcodes.IF_ICMPLE,
            Opcodes.IFEQ, Opcodes.IFNE, Opcodes.IFLT, Opcodes.IFGE,
            Opcodes.IFGT, Opcodes.IFLE, Opcodes.IFNULL, Opcodes.IFNONNULL,
            Opcodes.ATHROW -> true
            else -> false
        }
    }

    private fun intConstInsn(value: Int): AbstractInsnNode {
        return when (value) {
            -1 -> InsnNode(Opcodes.ICONST_M1)
            0 -> InsnNode(Opcodes.ICONST_0)
            1 -> InsnNode(Opcodes.ICONST_1)
            2 -> InsnNode(Opcodes.ICONST_2)
            3 -> InsnNode(Opcodes.ICONST_3)
            4 -> InsnNode(Opcodes.ICONST_4)
            5 -> InsnNode(Opcodes.ICONST_5)
            in Byte.MIN_VALUE..Byte.MAX_VALUE -> IntInsnNode(Opcodes.BIPUSH, value)
            in Short.MIN_VALUE..Short.MAX_VALUE -> IntInsnNode(Opcodes.SIPUSH, value)
            else -> LdcInsnNode(value)
        }
    }
}
