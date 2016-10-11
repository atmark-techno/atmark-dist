#!/bin/sh

CODEC=$1

ACM_CERT_SYSFS_DIR="/sys/${DEVPATH}"

FIRMWARE_DIR="/opt/firmware/acm"
LICENSE_DIR="/opt/license"

VENDOR_PUB_KEY="${FIRMWARE_DIR}/armadillo_pubkey.pem"
APP_PUB_KEY="${FIRMWARE_DIR}/acm_pubkey.pem"
APP_KEY_HASH_SIG="${FIRMWARE_DIR}/acm_pubkey.hash.armadillo.sign"
APP_HASH_SIG_PTN="${FIRMWARE_DIR}/acm-${CODEC}-*.fw.hash.acm.sign"
UID_SIG_PTN="${LICENSE_DIR}/0000000000000000*.uid.acm.sign"

store_pubkey() {
    local key=$1
    local type=$2

    convert_pubkey ${key}  > ${ACM_CERT_SYSFS_DIR}/${type}
    convert_pubkey -n ${key} >  ${ACM_CERT_SYSFS_DIR}/${type}_n
    convert_pubkey -e ${key} >  ${ACM_CERT_SYSFS_DIR}/${type}_e
}

UID_SIG=$(ls ${UID_SIG_PTN})
APP_HASH_SIG=$(ls ${APP_HASH_SIG_PTN})

echo 0 > ${ACM_CERT_SYSFS_DIR}/cert_control

store_pubkey ${VENDOR_PUB_KEY} cert_vendor_key
store_pubkey ${APP_PUB_KEY} cert_app_key

cat "${APP_KEY_HASH_SIG}" > ${ACM_CERT_SYSFS_DIR}/cert_app_key_hash_sig
cat "${APP_HASH_SIG}" > ${ACM_CERT_SYSFS_DIR}/cert_app_hash_sig
cat "${UID_SIG}" > ${ACM_CERT_SYSFS_DIR}/cert_uid_sig

echo 1 > ${ACM_CERT_SYSFS_DIR}/cert_control ||
	echo "failed to certification" >&2

exit 0
