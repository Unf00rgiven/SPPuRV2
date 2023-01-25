#!/bin/sh

scriptdir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
basedir=${scriptdir}/../..

# Get the tutorial name from the script directory
tutorial=${scriptdir##*/}

if [ $# -ne 2 ]; then
    echo "usage: build.sh <pi-model> <exmaple>" >&2
    echo "       pi-model options: rpi0, rpi1, rpi1bp, rpi2, rpi3, rpi4, rpibp\n       example options: v8_0, v8_1a, v8_1b, v8_2a, v8_2b, v8_3, v8_4" >&2
    exit 1
fi

# The raspberry pi model we're targetting
model="${1}"

# The example we're targetting
example="${2}"

# Set toolchain
toolchain="arm-none-eabi-"

# Make sure we have the c compiler we need
gcc_version=$(${toolchain}gcc --version)

if [ $? -ne 0 ]; then
    echo "Cannot find ${toolchain}gcc executable - please make sure it's present in your path" >&2
    echo "PATH: ${PATH}" >&2
    exit 1
fi

# Common CFLAGS
ccflags="-g"
ccflags="${ccflags} -nostartfiles"
ccflags="${ccflags} -mfloat-abi=softfp"
ccflags="${ccflags} -O0"

# Whatever specific flags we use should also include the common c flags
cflags="${ccflags}"

# Determine if the model is a b+ model or not
# We need to know if the raspberry pi model is b+ because the early RPI1 models have a different IO
# pin layout between the original units and the later B+ units. Similarly the RPI3 models have a
# different IO arrangement on the 3B+ units compared to the original 3B. We need to be able to
# be able to adjust the code for whichever model we're targetting
case "${model}" in
    *bp) cflags="${cflags} -DIOBPLUS" ;;
esac

case "${model}" in
    rpi0*)
        cflags="${cflags} -DRPI0"
        cflags="${cflags} -mfpu=vfp"
        cflags="${cflags} -march=armv6zk"
        cflags="${cflags} -mtune=arm1176jzf-s"
        ;;
    rpi1*)
        cflags="${cflags} -DRPI1"
        cflags="${cflags} -mfpu=vfp"
        cflags="${cflags} -march=armv6zk"
        cflags="${cflags} -mtune=arm1176jzf-s"
        ;;

    rpi2*)
        cflags="${cflags} -DRPI2"
        cflags="${cflags} -mfpu=neon-vfpv4"
        cflags="${cflags} -march=armv7ve"
        cflags="${cflags} -mtune=cortex-a7"
        ;;

    rpi3*)
        cflags="${cflags} -DRPI3"
        cflags="${cflags} -mfpu=crypto-neon-fp-armv8"
        cflags="${cflags} -march=armv8-a+crc"
        cflags="${cflags} -mcpu=cortex-a53"
        ;;

    rpi4*)
        cflags="${cflags} -DRPI4"
        cflags="${cflags} -mfpu=crypto-neon-fp-armv8"
        cflags="${cflags} -march=armv8-a+crc"
        cflags="${cflags} -mcpu=cortex-a72"
        ;;

    *) echo "Unknown model type ${model}" >&2 && exit 1
        ;;
esac

# Check example
if [ "${example}" != "v8_0" -a \
     "${example}" != "v8_1a" -a \
     "${example}" != "v8_1b" -a \
     "${example}" != "v8_2a" -a \
     "${example}" != "v8_2b" -a \
     "${example}" != "v8_3" -a \
     "${example}" != "v8_4" ]; then
  echo "Unknown example ${example}" >&2 && exit 1
fi

# Create output directory
mkdir -p ${scriptdir}/out

cflags="${cflags} -I${scriptdir}/kernel"

# Linker flags to pass through the compiler
lflags=""
lflags="${lflags} -Wl,-T,${scriptdir}/rpi.x"

kernel_elf="${scriptdir}/out/kernel.${tutorial}.${model}.elf"
kernel_img="${scriptdir}/out/kernel.${tutorial}.${model}.img"

kernel_elf_std="${scriptdir}/out/kernel.elf"
kernel_img_std="${scriptdir}/out/kernel.img"

kernel_asm="${scriptdir}/out/kernel.asm"
kernel_nm="${scriptdir}/out/kernel.nm"

printf "%s\n" "${toolchain}gcc ${cflags} ${lflags} ${scriptdir}/kernel/*.S ${scriptdir}/kernel/*.c ${scriptdir}/vezbe/${example}.c -o ${kernel_elf}"

# Do the compilation
${toolchain}gcc ${cflags} ${lflags} ${scriptdir}/kernel/*.S ${scriptdir}/kernel/*.c ${scriptdir}/vezbe/${example}.c -o ${kernel_elf}

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile!" >&2
    exit 1
fi

printf "%s\n" "${toolchain}objcopy ${kernel_elf} -O binary ${kernel_img}"
${toolchain}objcopy ${kernel_elf} -O binary ${kernel_img}

# Copy img and elf file to standard named files
printf "%s\n" "${kernel_elf} > ${kernel_elf_std}"
cp ${kernel_elf} ${kernel_elf_std}
printf "%s\n" "${kernel_img} > ${kernel_img_std}"
cp ${kernel_img} ${kernel_img_std}

# Dissasembly
printf "%s\n" "Disassembling ${kernel_elf_std} to ${kernel_asm}"
${toolchain}objdump -D ${kernel_elf_std} > ${kernel_asm}
${toolchain}nm ${kernel_elf_std} > ${kernel_nm}

