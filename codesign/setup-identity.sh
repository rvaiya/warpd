#!/bin/sh
#
# Create a private, self-signed code signing identity for local warpd builds.
#
# macOS ties the Accessibility permission to the binary's code signature.
# Signing every build with the same identity means the permission survives
# rebuilds. The private key is generated on this machine and never leaves
# it, so nobody else can produce binaries which inherit warpd's permissions.
#
# Usage: setup-identity.sh [keychain]   (default: your login keychain)

set -e

NAME=${SIGN_IDENTITY:-warpd-dev}
KEYCHAIN=${1:-$HOME/Library/Keychains/login.keychain-db}
OPENSSL=/usr/bin/openssl # LibreSSL, produces PKCS#12 files `security` can import

if security find-certificate -c "$NAME" "$KEYCHAIN" >/dev/null 2>&1; then
	echo "Signing identity '$NAME' already exists in $KEYCHAIN, nothing to do."
	exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/cert.cnf" <<EOF
[req]
distinguished_name = dn
x509_extensions = ext
prompt = no
[dn]
CN = $NAME
[ext]
basicConstraints = critical, CA:false
keyUsage = critical, digitalSignature
extendedKeyUsage = critical, codeSigning
EOF

# Throwaway passphrase, only protects the PKCS#12 file in transit to the keychain.
PASS=$($OPENSSL rand -hex 16)

$OPENSSL req -x509 -newkey rsa:2048 -nodes -days 3650 -config "$TMP/cert.cnf" \
	-keyout "$TMP/key.pem" -out "$TMP/cert.pem" 2>/dev/null
$OPENSSL pkcs12 -export -inkey "$TMP/key.pem" -in "$TMP/cert.pem" \
	-name "$NAME" -out "$TMP/id.p12" -passout "pass:$PASS"

# -T: allow codesign to use the key without prompting every build.
security import "$TMP/id.p12" -k "$KEYCHAIN" -P "$PASS" -T /usr/bin/codesign >/dev/null

echo "Created signing identity '$NAME' in $KEYCHAIN."
echo "If macOS asks whether codesign may use the key on the next build, choose 'Always Allow'."
