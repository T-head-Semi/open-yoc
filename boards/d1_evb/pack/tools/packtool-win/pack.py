# -*- coding:utf-8 -*-
import os
import sys
import glob
import shutil
import re
import subprocess
# import argparse

# parser = argparse.ArgumentParser()
# parser.add_argument("--debug", action='store_true', default=False, help="enable debug print")
# args = parser.parse_args()

# top = os.getcwd()
platform = {}
img_name = None
work_dir = None
pack_tools_dir = None
udisk_size = 0

debug_en = 0
rtos_origin_img = ""
fs_img = ""
out_dir = ""
mk_generated_imgs_path = ""
sys_partition_file = ""
product_file = ""

pack_top_dir = None

phoenixplugin_file_list = [
    "tools\\phoenixplugin\\*.fex"
]

common_config_file_list = [
    "arisc.fex", "config.fex", "split_xxxx.fex", "sunxi.fex", "sys_config.fex",
    "package_boot0.cfg", "rootfs.ini"
]

special_fex_config_file = {
    'nor': ["sys_config_nor.fex", "sys_partition_nor.fex", "sys_partition_nor_dump.fex", "cardscript.fex"],
    'nand': ["sys_config_nand.fex", "sys_partition_nand.fex", "sys_partition_nand_dump.fex"],
    'card': ["cardscript.fex", "sys_config_card.fex", "sys_partition_card.fex", "sys_partition_card_dump.fex"],
    'card_product': ["cardscript_product.fex", "sys_config_card_product.fex", "sys_partition_card_product.fex"]
}

special_cfg_config_file = {
    'nor': ["image_nor.cfg", "package_uboot_nor.cfg"],
    'nand': ["image_nand.cfg", "package_uboot_nand.cfg"],
    'card': ["image_card.cfg", "package_uboot_card.cfg"],
    'card_product': ["image_card_product.cfg", "package_uboot_card_product.cfg"]
}

vesion_file_list = [
    "version_base.mk"
]

def win_sys_exit(code):
    cmd = "exit /B 1"
    os.system(cmd)

def debug_print(*objects, sep=' ', end='\n', file=sys.stdout, flush=False):
    global debug_en
    if int(debug_en) > 0:
        print(*objects, sep=' ', end='\n', file=sys.stdout, flush=False)


def check_file_exists(file):
    if not os.path.exists(file):
        print('%s not exists!' % file)
        win_sys_exit(1)


def cp(src, dst):
    check_file_exists(src)
    debug_print("%-50s \t->\t %s" % (src, dst))
    shutil.copy(src, dst)


# 读取平台信息
def read_platform_info():
    global platform
    global work_dir
    global img_name
    global pack_tools_dir
    # global top
    global out_dir

    # top = os.path.abspath(top)
    # debug_print(top)
    plat_file = pack_tools_dir + "\\platform.txt"
    debug_print(plat_file)
    if not os.path.exists(plat_file):
        print("platform file not exists!")
        win_sys_exit(1)
    with open(plat_file, 'r') as f:
        for line in f:
            line = line.rstrip()
            info = line.split('=')
            # print("info = %s" % info)
            if len(info) == 2:
                platform[info[0]] = info[1]

    work_dir = out_dir + "\\image_xxx_win"
    debug_print("work_dir = %s" % work_dir)
    try:
        # shutil.rmtree(work_dir, ignore_errors=True)
        if os.path.isdir(work_dir):
            cmd = "rd /S /Q " + "\"" + work_dir + "\"" + " >nul"
            debug_print(cmd)
            os.system(cmd)
        os.mkdir(work_dir)
        os.chdir(work_dir)
    except Exception as e:
        print(str(e))
        win_sys_exit(1)
    debug_print("goto work_dir")

    img_name = platform['platform'] + "_" + \
        platform['board'] + "_" + platform['debug']

    if not platform['sigmode'] == "none":
        img_name += "_" + platform['sigmode']
    if platform['mode'] == "dump":
        img_name += "_" + platform['mode']
    if platform['securemode'] == "secure":
        img_name += "_" + platform['securemode']
    if platform['torage_type'] == 'nor':
        img_name += "_" + platform['nor_volume'] + "Mnor"

    img_name += ".img"
    debug_print("img_name = ", img_name)

    sys.path.append(pack_tools_dir)


def copy_pack_tools():
    global pack_tools_dir
    global work_dir

    image_dir = work_dir

    for file in glob.glob(pack_tools_dir + "\\*"):
        dst_file = os.path.join(image_dir, os.path.basename(file))
        shutil.copy(file, dst_file)


def remove_pack_tools():
    global pack_tools_dir
    global work_dir

    image_dir = work_dir + "\\"
    for file in glob.glob(pack_tools_dir + "\\*"):
        file_name = image_dir + os.path.basename(file)
        cmd = "del /F /Q /S " + "\"" + file_name + "\"" + " >nul"
        os.system(cmd)
        # os.remove(file_name)


def copy_commond_files(files, src_dir, dst_dir):
    for item in files:
        src = os.path.join(src_dir, item)
        dst = os.path.join(dst_dir, item)
        if os.path.exists(src):
            shutil.copy(src, dst)

def copy_specital_files(id, files, src_dir, dst_dir):
    if id not in files.keys():
        print(id + "Not support!!!")
        win_sys_exit(1)
    for item in files[id]:
        src = os.path.join(src_dir, item)
        dst = os.path.join(dst_dir, item)
        if os.path.exists(src):
            shutil.copy(src, dst)


def copy_files():
    global rtos_origin_img
    global fs_img
    global sys_partition_file    
    global pack_top_dir
    global work_dir
    global mk_generated_imgs_path

    image_dir = work_dir

    config_file_dir = pack_top_dir + "\\projects\\" + "\\configs\\"
    version_file_dir = pack_top_dir + "\\projects\\" + "\\version\\"
    boot_file_dir = pack_top_dir + "\\projects\\" + "\\bin\\"
    epos_file = rtos_origin_img

    debug_print("\n\n----------------------------copy phoenixplugin fex file------------------------------")
    for item in phoenixplugin_file_list:
        for file in glob.glob(pack_top_dir + "\\" + item):
            name = os.path.basename(file)
            dst = os.path.join(work_dir, name)
            shutil.copyfile(file, dst)

    debug_print("\n\n----------------------------copy config fex file------------------------------")
    copy_commond_files(common_config_file_list, config_file_dir, image_dir)
    copy_specital_files(platform['torage_type'], special_fex_config_file, config_file_dir, image_dir)

    debug_print("\n\n----------------------------copy config cfg file------------------------------")
    copy_specital_files(platform['torage_type'], special_cfg_config_file, config_file_dir, image_dir)

    debug_print("\n\n----------------------------copy version mk file------------------------------")
    copy_commond_files(vesion_file_list, version_file_dir, image_dir)
    debug_print("\n\n----------------------------copy boot bin file------------------------------")

    copy_file = boot_file_dir + "fes1_" + platform["chip"] + ".bin"
    dst = image_dir + "\\fes1.fex"
    shutil.copy(copy_file, dst)

    dst = image_dir + "\\epos.img"
    src = epos_file
    shutil.copy(src, dst)

    if platform["torage_type"] == "card_product":
        copy_file = boot_file_dir + "boot0_" + platform["chip"] + "_card.bin"
        dst = image_dir + "\\boot0_card_product.fex"
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)

    elif platform["torage_type"] == "nor":
        # copy_file = boot_file_dir + "boot0_" + platform["chip"] + "_nor.bin"
        dst = image_dir + "\\boot0_nor.bin"
        src = os.path.join(mk_generated_imgs_path, "data\\boot0")
        shutil.copy(src, dst)

        dst = image_dir + "\\boot0_nor.fex"
        src = os.path.join(mk_generated_imgs_path, "data\\boot0")
        shutil.copy(src, dst)
        dst = image_dir + "\\boot_nor.fex"
        src = os.path.join(mk_generated_imgs_path, "data\\boot")
        shutil.copy(src, dst)

        copy_file = boot_file_dir + "u-boot_" + platform["chip"] + "_nor"
        if not platform["debug"] and platform["debug"] != 'none':
            copy_file += "_" + platform["debug"]
        copy_file += ".bin"
        dst = image_dir + "\\u-boot_nor.fex"
        shutil.copy(copy_file, dst)
    else:
        copy_file = boot_file_dir + "boot0_" + platform["chip"] + "_nand.bin"
        dst = image_dir + "\\boot0_nand.fex"
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)

        copy_file = boot_file_dir + "u-boot_" + platform["chip"] + "_nand.bin"
        dst = image_dir + "\\u-boot_nand.fex"
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)
    # boot0_card.fex must copy in order to support card burn
    copy_file = boot_file_dir + "boot0_" + platform["chip"] + "_card.bin"
    dst = image_dir + "\\boot0_card.fex"
    shutil.copy(copy_file, dst)

    if platform["securemode"] == "secure":
        debug_print("\n\n----------------------------copy secure boot file------------------------------")
        copy_file = boot_file_dir + "sboot_" + platform["chip"] + ".bin"
        dst = image_dir + "\\sboot.bin"
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)
    
    if os.path.exists(fs_img):
        dst = image_dir + "\\littlefs.fex"
        shutil.copy(fs_img, dst)
    dst = image_dir + "\\sys_partition_nor.fex"
    shutil.copy(sys_partition_file, dst)

def del_match(target, match):
    lines = None

    with open(target, 'r', encoding='utf8') as f:
        lines = f.readlines()
    with open(target, 'w') as f:
        for l in lines:
            if re.match(match, l):
                continue
            f.write(l)


def set_img_name():
    global img_name
    image_cfg_file = work_dir + "\\image_" + platform["torage_type"] + ".cfg"

    lines = None

    with open(image_cfg_file, 'r') as f:
        lines = f.readlines()
    with open(image_cfg_file, 'w') as f:
        for l in lines:
            if re.match("(^imagename.*)|(^;.*)", l):
                continue
            f.write(l)
        f.write("imagename = " + img_name)
        f.write("\n\n")


def format_file(file, toformat='unix2dos'):
    if toformat == 'unix2dos':
        src = b'\r'
        dst = b'\r\n'
    else:
        src = b'\r\n'
        dst = b'\n'

    with open(file, 'rb+') as f1:
        byt = f1.read()

        byt = byt.replace(src, dst)
        tempfile = open(file + toformat, 'wb+')
        tempfile.write(byt)
        tempfile.close()

    os.remove(file)
    os.rename(file + toformat, file)


def copy_image_dir_file(src, dst):
    global work_dir

    image_dir = work_dir + "\\"

    src = image_dir + src
    dst = image_dir + dst

    src = os.path.relpath(src)
    dst = os.path.relpath(dst)
    if src != dst:
        cp(src, dst)


def mv_image_dir_file(src, dst):
    global work_dir

    image_dir = work_dir + "\\"

    if src != dst:
        debug_print("%s -mv-> %s" % (src, dst))
        shutil.move(src, dst)


def exec_cmd(cmd, *args):
    global pack_tools_dir
    global work_dir
    global debug_en

    image_dir = work_dir + "\\"
    arg = " "
    exec_file = cmd
    #exec_file = os.path.relpath(exec_file)

    for a in args:
        if type(a) is str:
            a = os.path.relpath(a)
            arg += a + " "
        else:
            arg += str(a) + " "

    cmd_line = exec_file + arg

    if int(debug_en) == 0:
        cmd_line += " >/nul"

    debug_print(cmd_line + "\n")

    if not os.path.exists(exec_file):
        print("!!!exec file %s not exits!!!" % (cmd))
        print("cmd line = %s" % cmd_line)
        win_sys_exit(1)

    proc = subprocess.Popen(cmd_line, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, shell=True)
    outinfo, errinfo = proc.communicate()
    info = str(outinfo, encoding="utf-8")
    if 'error' in info or 'failed' in info:
        print("exec %s error!" % cmd_line)
        print(info)
        win_sys_exit(1)


# convert the fex file to bin file
def script_file(storage):
    global pack_tools_dir
    global work_dir

    image_dir = work_dir + "\\"

    # prepare source files
    config_file_src = "sys_config_" + storage + ".fex"
    config_file_dst = "sys_config_" + storage + ".bin"

    del_match(image_dir + config_file_src, '(^;.*)|(^ +;.*)|(^$)')

    check_file_exists(config_file_src)
    exec_cmd("mode_script.exe", config_file_dst, config_file_src)

    cp(image_dir + config_file_dst, image_dir + "config_nor.fex")
    cp(image_dir + config_file_dst, image_dir + "sys_config.bin")

    sys_partition__src = "sys_partition_" + storage + ".fex"
    sys_partition_dst = "sys_partition_" + storage + ".bin"
    check_file_exists(sys_partition__src)
    exec_cmd("mode_script.exe", sys_partition_dst, sys_partition__src)


def update_boot0(storage='nor'):
    global work_dir

    image_dir = work_dir + "\\"
    config_file = "sys_config_" + storage + ".bin"

    output_file = image_dir
    storage_type = "SPINOR_FLASH"

    if storage == "nor":
        output_file += "boot0_nor.fex "
        storage_type = "SPINOR_FLASH"
    elif storage == "nand":
        output_file += "boot0_nand.fex "
        storage_type = "NAND "
    elif storage == "card":
        output_file += "boot0_card.fex "
        storage_type = "SDMMC_CARD"
    else:
        output_file += "boot0_card_product.fex "
        storage_type = "SDMMC_CARD"

    input_file = image_dir + config_file

    check_file_exists(input_file)
    # exec_cmd("update_boot0.exe", output_file, input_file, storage_type)
    exec_cmd("update_boot0.exe", output_file, input_file, storage_type)
    # FIXME: just for test
    # print(work_dir)
    # other_boot0_img = os.path.join(work_dir, "..\\image_xxx\\boot0_nor.fex")
    # print(other_boot0_img)
    # check_file_exists(other_boot0_img)
    # shutil.copy(other_boot0_img, "boot0_nor.fex")


def package_boot0(storage):
    global work_dir

    image_dir = work_dir + "\\"

    src = image_dir + "package_boot0.cfg"
    dst = "melis_pkg_" + storage + ".fex"
    if not os.path.exists(src):
        print("package_boot0.cfg file not exits")
        win_sys_exit(1)
    check_file_exists("epos.img")
    exec_cmd("compress.exe", "-c", "epos.img", "epos.img.lzma")
    exec_cmd("lz4.exe", "epos.img", "epos-lz4.img")
    check_file_exists(src)
    exec_cmd("dragonsecboot.exe", "-pack", src)
    check_file_exists("boot_package.fex")
    mv_image_dir_file("boot_package.fex", dst)


def update_fes1(storage):
    global work_dir

    image_dir = work_dir + "\\"
    config_file = "sys_config_" + storage + ".bin"

    src = image_dir + config_file
    dst = image_dir + "fes1.fex"
    if not os.path.exists(src):
        print("sys_config.fex file not exits")
        return

    exec_cmd("update_fes1.exe", dst, src)


def update_uboot(storage):
    global work_dir

    image_dir = work_dir + "\\"
    config_file = "sys_config_" + storage + ".bin"

    src = image_dir + config_file
    dst = image_dir + "u-boot_" + storage + ".fex"
    if not os.path.exists(src):
        print("sys_config.fex file not exits")
        return

    check_file_exists(src)
    exec_cmd("update_uboot.exe", "-no_merge", dst, src)


def package_uboot(storage):
    global work_dir

    image_dir = work_dir + "\\"
    file_name = "package_uboot_" + storage + ".cfg"
    src = image_dir + file_name
    dst = "boot_pkg_uboot_" + storage + ".fex"
    temp = image_dir + "boot_package.fex"
    config_file = image_dir + "sys_config_" + storage + ".bin"

    if not os.path.exists(src):
        print("package_boot0.cfg file not exits")
        return

    check_file_exists(src)
    exec_cmd("dragonsecboot.exe", "-pack", src)

    check_file_exists(temp)
    exec_cmd("update_toc1.exe", temp, config_file)
    mv_image_dir_file(temp, dst)


def update_env():
    global work_dir

    image_dir = work_dir + "\\"

    src = image_dir + "env.cfg"
    dst = image_dir + "env.fex"

    if not os.path.exists(src):
        print("package_boot0.cfg file not exits")
        return

    check_file_exists(src)
    exec_cmd("mkenvimage.exe", "-r -p 0x00 -s 4096 -o", dst, src)


def get_part_info(file, total):
    start = False
    lines = None
    info = {}
    name = None
    _udisk_size = 0

    debug_print("get partition info from ", file)

    tempfile = file + '.tmp'
    with open(file, 'r', encoding='utf8') as f:
        lines = f.readlines()
    with open(tempfile, 'w') as f:
        for l in lines:
            if re.match("(^;.*)|(^ +;.*)|(^$)", l):
                continue
            if start:
                f.write(l)
            if "[partition_start]" in l:
                start = True
    with open(tempfile, 'r') as f:
        lines = f.readlines()
    part_str = ""
    for l in lines:
        if "[partition]" in l:
            part_str += '~'
        else:
            part_str += l
    for item in re.split('~', part_str):
        if item != "":
            part_info = re.split('=|\n+', item)
            del part_info[-1]
            for i in range(len(part_info)):
                part_info[i] = part_info[i].strip()

            if len(part_info) % 2 != 0:
                print("parse part info failed!")
                return None
            # find partition name
            for i in range(0, len(part_info), 2):
                if part_info[i].lower() == 'name':
                    name = part_info[i+1]
                    break
            iinfo = {}
            for i in range(0, len(part_info), 2):
                if part_info[i].lower() != 'name':
                    iinfo[part_info[i]] = part_info[i+1]
            info[name] = iinfo
    sum = 0
    for k, v in info.items():
        if k != 'UDISK':
            if 'size' in v.keys():
                s = int(v['size'])
                v['size'] = str(int(s / 2))
                sum += int(s / 2)
            else:
                print("parse part info failed!")
                return None
    for k, v in info.items():
        if k == 'UDISK':
            v['size'] = str(total - sum)
            _udisk_size = (total - sum) * 1024 / 512

    return info, _udisk_size


def perpare_for_nor():
    global work_dir
    global udisk_size

    image_dir = work_dir + "\\"

    partition_file = image_dir + "sys_partition_nor.fex"
    part_kb_for_nor = int(platform['nor_volume'])*1024 - 64

    part_info, udisk_size = get_part_info(partition_file, part_kb_for_nor)
    debug_print(part_info)
    # # make fs
    # name = top + "\\projects\\" + platform['board'] + "\\data\\UDISK"
    # down_file = re.sub(r'"', "", part_info['ROOTFS']['downloadfile'])
    # if not os.path.exists(name):
    #     print('not found %s to creating %s' % (name, down_file))
    #     return
    # # minfs make name down_file rootfs.ini
    # rootfs_ini = image_dir + "rootfs.ini"

    # check_file_exists(rootfs_ini)
    # exec_cmd("minfs.exe", "make",  name, down_file, rootfs_ini)

    return part_info


def updarte_mbr(storage, udisk_size, full_size):
    global work_dir

    image_dir = work_dir + "\\"

    sys_partition_file = image_dir + "sys_partition_" + storage + ".bin"
    dlinfo_file = image_dir + "dlinfo.fex"
    gpt_file = image_dir + "sunxi_gpt.fex"
    mbr_source_file = image_dir + "sunxi_mbr_" + storage + ".fex"
    boot0_file_name = image_dir + "boot0_.fex"

    # update mbr file
    if not os.path.exists(sys_partition_file):
        print('not exists partition.fex file')
        win_sys_exit(1)

    check_file_exists(sys_partition_file)
    if storage == 'nor':
        exec_cmd("update_mbr.exe",  sys_partition_file,
                 mbr_source_file, dlinfo_file, 1)
    else:
        exec_cmd("update_mbr.exe",  sys_partition_file,
                 mbr_source_file, dlinfo_file, 4)
    # convert mbr to get
    debug_print("----------------mbr convert to gpt start---------------------")
    check_file_exists(mbr_source_file)
    exec_cmd("convert_gpt.exe", "-s",  mbr_source_file,
             "-o", gpt_file, "-l 96 -u 8")


def create_img(part_info):
    global work_dir
    global platform
    global img_name

    debug_print("\n\n----------------------------create image------------------------------")

    storage = platform['torage_type']
    image_dir = work_dir + "\\"

    debug_print("image name = %s" % img_name)
    # image_dst = work_dir + "\\" + img_name
    image_src = img_name

    img_cfg_file = image_dir + "image_" + storage + ".cfg"
    sys_partition_file = image_dir + "sys_partition_" + storage + ".fex"

    # check file exists
    check_file_exists(img_cfg_file)
    check_file_exists(sys_partition_file)

    format_file(img_cfg_file, 'dos2unix')
    with open(img_cfg_file, 'r') as f:
        for line in f:
            if re.search('filename[ ]+=[ ]".*"', line):
                filename = re.sub('"', '', line.split(',')[
                                  0].split('=')[1]).strip()
                filedir = os.path.relpath(image_dir) + "\\" + filename
                if not os.path.exists(filedir):
                    print("%-30s <Failed>" % filename)
                else:
                    debug_print("%-30s <Success>" % filename)
    if part_info == None:
        print('partition error!')
        win_sys_exit(1)
    for k, v in part_info.items():
        if 'downloadfile' in v.keys():
            filename = v['downloadfile']
            filename = re.sub('"', '', filename)
            filedir = os.path.relpath(image_dir) + "\\" + filename
            if not os.path.exists(filedir):
                print("%-30s <Failed>" % filename)
            else:
                debug_print("%-30s <Success>" % filename)

    exec_cmd("dragon.exe",  img_cfg_file, sys_partition_file)

    if not os.path.exists(image_src):
        print("Create image failed!")
        win_sys_exit(1)
    image_dst = out_dir + "\\yoc_rtos_" + platform['nor_volume'] + "M.img"
    if os.path.exists(image_dst):
        cmd = "del /F /Q /S " + "\"" + image_dst + "\"" + " >nul"
        os.system(cmd)
    cmd = "move /Y " + "\"" + image_src + "\" \"" + image_dst + "\"" + " >nul"
    os.system(cmd)
    print('Create %s Success!' % image_dst)


def do_update(storage):
    debug_print("\n\n----------------------------do update------------------------------")
    set_img_name()
    script_file(storage)
    update_boot0(storage)
    package_boot0(storage)
    update_fes1(storage)
    update_uboot(storage)
    package_uboot(storage)
    # update_env()


def do_finish(storage, part_info):
    global udisk_size

    updarte_mbr(storage, udisk_size, platform['nor_volume'])
    create_img(part_info)

def variables_init():
    global debug_en
    global rtos_origin_img
    global fs_img
    global out_dir
    global mk_generated_imgs_path
    global sys_partition_file
    global product_file
    global pack_top_dir
    global pack_tools_dir

    debug_en = sys.argv[1]
    rtos_origin_img = sys.argv[2]
    fs_img = sys.argv[3]
    out_dir = sys.argv[4]
    mk_generated_imgs_path = sys.argv[5]
    sys_partition_file = sys.argv[6]
    product_file = sys.argv[7]

    if getattr(sys, 'frozen', False):
        pack_tools_dir = os.path.dirname(os.path.realpath(sys.executable))
    elif __file__:
        pack_tools_dir = os.path.split(os.path.realpath(__file__))[0]
    debug_print("pack_tools_dir: %s" % pack_tools_dir)
    pack_top_dir = os.path.join(pack_tools_dir, "..\\..")
    debug_print("pack_top_dir: %s" % pack_top_dir)
    if not os.path.isdir(pack_top_dir):
        print("the pack dir is not exist")
        win_sys_exit(0)
    if not rtos_origin_img or not os.path.exists(rtos_origin_img):
        print("the origin rtos image file is not exist.")
        win_sys_exit(0)
    if not sys_partition_file or not os.path.exists(sys_partition_file):
        print("the sys partition file is not exist.")
        win_sys_exit(0)
    if not product_file or not os.path.exists(product_file):
        print("the product.exe is not exist.")
        win_sys_exit(0)
    if not out_dir or not os.path.isdir(out_dir):
        print("the out dir is not exist.")
        win_sys_exit(0)
    if not mk_generated_imgs_path or not os.path.isdir(mk_generated_imgs_path):
        print("the generated dir is not exist.")
        win_sys_exit(0)
    debug_print("variable init ok.")

def do_imageszip():
    global mk_generated_imgs_path
    global product_file

    debug_print("create images.zip start.")
    check_file_exists(product_file)
    images_zip = os.path.join(mk_generated_imgs_path, "images.zip")
    check_file_exists("melis_pkg_nor.fex")
    shutil.copy("melis_pkg_nor.fex", os.path.join(mk_generated_imgs_path, "data/prim"))
    if os.path.exists("littlefs.fex"):
        shutil.copy("littlefs.fex", os.path.join(mk_generated_imgs_path, "data/lfs"))
    # exec_cmd(product_file, "image", images_zip, "-i", os.path.join(mk_generated_imgs_path, "data"), "-l", "-p")
    cmd = "".join("\"%s\" %s %s %s %s %s " % (product_file, "image", images_zip, "-i", os.path.join(mk_generated_imgs_path, "data"), "-l -p"))
    os.system(cmd)
    # exec_cmd(product_file, "image", images_zip, "-e", mk_generated_imgs_path, "-x")
    cmd = "".join("\"%s\" %s %s %s %s %s " % (product_file, "image", images_zip, "-e", mk_generated_imgs_path, "-x"))
    os.system(cmd)
    # exec_cmd(product_file, "image", images_zip, "-e", mk_generated_imgs_path)
    cmd = "".join("\"%s\" %s %s %s %s " % (product_file, "image", images_zip, "-e", mk_generated_imgs_path))
    os.system(cmd)
    check_file_exists(os.path.join(mk_generated_imgs_path, "prim"))
    shutil.copy(os.path.join(mk_generated_imgs_path, "prim"), "melis_pkg_nor.fex")
    check_file_exists(os.path.join(mk_generated_imgs_path, "imtb"))
    shutil.copy(os.path.join(mk_generated_imgs_path, "imtb"), "imtb.fex")
    debug_print("create images.zip finish.")


if __name__ == "__main__":
    part_info = None
    variables_init()
    read_platform_info()
    copy_files()
    copy_pack_tools()
    do_update(platform['torage_type'])
    if platform['torage_type'] == "nor":
        part_info = perpare_for_nor()
    do_imageszip()
    do_finish(platform['torage_type'], part_info)
    remove_pack_tools()
