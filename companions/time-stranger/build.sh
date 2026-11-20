#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
package="$root/companions/time-stranger"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to Android NDK r28c}"
builder="${1:?Pass the path to build_dualscreen_package.py}"
case "$(uname -s)" in
  Darwin) tools="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64/bin" ;;
  Linux) tools="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin" ;;
  *) echo 'Build on macOS or Linux.' >&2; exit 1 ;;
esac
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/package/dualscreen"
cp "$package/dualscreen/manifest.json" "$work/package/dualscreen/"
cp "$package/README.md" "$work/package/"
"$tools/aarch64-linux-android28-clang++" -std=c++20 -O2 -Wall -Wextra -Werror \
  -shared -fPIC -fvisibility=hidden -static-libstdc++ -Wl,-z,max-page-size=16384 \
  -I "$root/src" "$package/010062E01FE0C000.cpp" -o "$work/010062E01FE0C000.so"
"$tools/llvm-strip" --strip-all "$work/010062E01FE0C000.so"
python3 "$builder" --package "$work/package" --output "$package/dist" \
  --version 0.1.0 --min-runtime 19 \
  --module "android-arm64-v8a=$work/010062E01FE0C000.so" \
  --build-id 135C83CDEA06AEC2D858E1BC7D53D72F7257166C000000000000000000000000
