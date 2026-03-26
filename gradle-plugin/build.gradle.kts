plugins {
    `kotlin-dsl`
    `java-gradle-plugin`
    `maven-publish`
}

group = "com.chaosprotector"
version = "2.0.0"

repositories {
    google()
    mavenCentral()
}

dependencies {
    implementation("com.android.tools.build:gradle-api:8.2.0")
    implementation("com.android.tools.build:gradle:8.2.0")
}

gradlePlugin {
    plugins {
        create("chaosProtectorAndroid") {
            id = "com.chaosprotector.android"
            implementationClass = "com.chaosprotector.android.ChaosProtectorPlugin"
            displayName = "ChaosProtector Android"
            description = "LLVM-based code obfuscation for Android native libraries"
        }
    }
}

publishing {
    repositories {
        maven {
            name = "local"
            url = uri(layout.buildDirectory.dir("repo"))
        }
    }
}
