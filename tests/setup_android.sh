#!/bin/bash
set -e
export ANDROID_HOME=$HOME/android-sdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export PATH=$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$ANDROID_HOME/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

mkdir -p $ANDROID_HOME/cmdline-tools
cd /tmp

# Download SDK tools if not present
if [ ! -f $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager ]; then
    echo "Downloading Android SDK command-line tools..."
    wget -q "https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip" -O cmdtools.zip
    unzip -qo cmdtools.zip -d $ANDROID_HOME/cmdline-tools/
    rm -rf $ANDROID_HOME/cmdline-tools/latest
    mv $ANDROID_HOME/cmdline-tools/cmdline-tools $ANDROID_HOME/cmdline-tools/latest
    rm cmdtools.zip
fi

echo "SDK tools: $(ls $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager)"

# Accept licenses and install NDK + platform
yes | sdkmanager --licenses > /dev/null 2>&1 || true
echo "Installing NDK and platform..."
sdkmanager "ndk;26.1.10909125" "platforms;android-34" "build-tools;34.0.0" 2>&1 | tail -5

echo "NDK installed at: $ANDROID_HOME/ndk/26.1.10909125"
echo "AArch64 clang: $(which aarch64-linux-android34-clang 2>/dev/null || echo 'checking...')"
ls $ANDROID_HOME/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android*-clang 2>/dev/null | head -1
echo "Setup complete!"
