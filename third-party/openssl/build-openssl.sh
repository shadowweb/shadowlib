#! /bin/bash

exit_code=1

current_dir=${PWD}
openssl_file=${PWD}/$1
build_dir="build-third-party"
openssl_dir=`basename ${openssl_file%.tar.gz}`

rm -rf ${build_dir}/${openssl_dir}
rm -rf ${build_dir}/lib/openssl
rm -rf ${build_dir}/include/openssl

mkdir -p ${build_dir} && cd ${build_dir} && tar zxf ${openssl_file}
if [ $? == 0 ]
then
    cd ${openssl_dir}
    ./config no-comp no-idea shared -DPURIFY -d
    if [ $? == 0 ]
    then
        make depend && make build_libs
        if [ $? == 0 ]
        then
            cd ${current_dir}
            mkdir -p ${build_dir}/lib/openssl && mkdir -p ${build_dir}/include
            if [ $? == 0 ]
            then
                ln -s ${PWD}/${build_dir}/${openssl_dir}/include/openssl    ${build_dir}/include/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libssl.a           ${build_dir}/lib/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libssl.so.1.0.0    ${build_dir}/lib/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libssl.so          ${build_dir}/lib/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libcrypto.a        ${build_dir}/lib/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libcrypto.so.1.0.0 ${build_dir}/lib/openssl &&
                ln -s ${PWD}/${build_dir}/${openssl_dir}/libcrypto.so       ${build_dir}/lib/openssl
                if [ $? == 0 ]
                then
                    exit_code=0
                fi
            fi
        fi
    fi
fi

exit $exit_code
