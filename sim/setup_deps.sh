#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."
ROOT=$(pwd)
mkdir -p "${ROOT}/third_party"
cd "${ROOT}/third_party"

clone() {
    local repo=$1
    local dir=$2
    local ref=$3

    if [ -d "${dir}" ]; then
        echo "[deps] ${dir} already present"
        return
    fi

    echo "[deps] cloning ${repo} @ ${ref}"
    git -c http.proxy= clone --depth 1 --branch "${ref}" \
        "https://ghfast.top/https://github.com/${repo}.git" "${dir}" \
        || git -c http.proxy= clone --depth 1 --branch "${ref}" \
            "https://github.com/${repo}.git" "${dir}"
}

clone lovyan03/LovyanGFX LovyanGFX master
clone lvgl/lvgl lvgl release/v9.5
clone Forairaaaaa/Mooncake Mooncake v2.3.3

echo "[deps] done"
