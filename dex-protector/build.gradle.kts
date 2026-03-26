plugins {
    kotlin("jvm") version "1.9.22"
    `java-library`
}

group = "com.chaosprotector"
version = "2.0.0"

repositories {
    mavenCentral()
}

dependencies {
    implementation("org.ow2.asm:asm:9.6")
    implementation("org.ow2.asm:asm-tree:9.6")
    implementation("org.ow2.asm:asm-commons:9.6")
    testImplementation(kotlin("test"))
}

tasks.test {
    useJUnitPlatform()
}
