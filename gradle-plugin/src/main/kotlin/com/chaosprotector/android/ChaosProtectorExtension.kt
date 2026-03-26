package com.chaosprotector.android

import org.gradle.api.provider.Property
import org.gradle.api.model.ObjectFactory
import javax.inject.Inject

/**
 * ChaosProtector Android Gradle DSL extension.
 *
 * Usage in build.gradle:
 * ```
 * chaosProtector {
 *     licenseKey = "CP-XXXX-XXXX-XXXX-XXXX"
 *     protections {
 *         stringEncryption = true
 *         controlFlowFlattening = true
 *         arithmetic = true
 *     }
 * }
 * ```
 */
open class ChaosProtectorExtension @Inject constructor(objects: ObjectFactory) {
    val licenseKey: Property<String> = objects.property(String::class.java).convention("")
    val enabled: Property<Boolean> = objects.property(Boolean::class.java).convention(true)
    val protections: ProtectionConfig = objects.newInstance(ProtectionConfig::class.java)

    fun protections(action: ProtectionConfig.() -> Unit) {
        action(protections)
    }
}

open class ProtectionConfig @Inject constructor(objects: ObjectFactory) {
    val stringEncryption: Property<Boolean> = objects.property(Boolean::class.java).convention(true)
    val controlFlowFlattening: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val opaqueConstants: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val arithmetic: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val antiHook: Property<Boolean> = objects.property(Boolean::class.java).convention(true)
    val indirectBranch: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val indirectCall: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val breakControlFlow: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val basicBlockDuplicate: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val functionOutline: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
    val opaqueFieldAccess: Property<Boolean> = objects.property(Boolean::class.java).convention(false)
}
