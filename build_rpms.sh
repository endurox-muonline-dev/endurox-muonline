#!/bin/bash
set -e

export MU_VERSION=1.0.0
export GIT_VERSION=`git rev-parse --short HEAD`

echo "Version $MU_VERSION"
QA_RPATHS='\$[ 0x0001 | 0x0010 ]' rpmbuild -bb rpmbuild/SPECS/endurox-muserver.spec