# -*- coding:utf-8 -*-
import os
import sys
import glob
import shutil
import re
import subprocess
import platform
import codecs
import argparse
import zipfile
try:
    import yaml
except:
    print("\n\nNot found pyyaml, please install: \nsudo pip install pyyaml")
    sys.exit(0)

class Package(object):
    def __init__(self, filename=None):
        self.filename = filename
        self.mtb_version = ''
        self.chip = ''
        self.diff = {}
        self.flash = {}
        self.partitions = {}

        if filename:
            self.load(filename)

    def yaml_load(self, filename):
        try:
            with codecs.open(filename, 'r', encoding= 'utf8') as fh:
                text = fh.read()
                return yaml.safe_load(text)
        except Exception as e:
            try:
                with codecs.open(filename, 'r', encoding= 'unicode_escape') as fh:
                    text = fh.read()
                    return yaml.safe_load(text)
            except Exception as e:
                print("(%s) in file:%s" % (str(e), filename))
                sys_exit(1)

    def load(self, filename):
        conf = self.yaml_load(filename)
        if not conf:
            return

        had_err = 0
        conf_str = ''
        for k, v in conf.items():
            if v:
                if isinstance(v, list):
                    while None in v:
                        v.remove(None)
                        had_err = 1
                    if had_err:
                        conf_str = k
                self.__dict__[k] = v

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--debug_en", type=int, default='0', help="enable debug print")
parser.add_argument("-r", "--rtos_origin_img", type=str, default='', help="the rtos image path")
parser.add_argument("-f", "--fs_data_dir", type=str, default='', help="the littlefs data dir")
parser.add_argument("-o", "--out_dir", type=str, default='', help="the out dir")
parser.add_argument("-m", "--mk_generated_imgs_path", type=str, default='', help="the genereated dir")
parser.add_argument("-c", "--config_yaml", type=str, default='', help="the config.yaml file")
parser.add_argument("-z", "--factory_zip", type=str, default='', help="the factory zip from occ")
iargs = parser.parse_args()

platformf = {}
img_name = None
work_dir = None
pack_tools_dir = None
udisk_size = 0

debug_en = 0
rtos_origin_img = ""
fs_img = ""
fs_data_dir = ''
out_dir = ""
mk_generated_imgs_path = ""
product_file = ""
config_yaml = ""
configs_dir = ""
factory_zip = None

pack_top_dir = None

phoenixplugin_file_list = [
    "tools/phoenixplugin/*.fex"
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

def isWindows():
    sys = platform.system()
    if sys == 'Windows':
        return True
    return False

def sys_exit(code):
    sys.exit(code)

def debug_print(*args, **keys):
    global debug_en
    if int(debug_en) > 0:
        for a in args:
            if sys.version_info.major == 2:
                if type(a) == unicode:
                    a = a.encode('utf8')
            print(a)

def check_file_exists(file):
    if not os.path.exists(file):
        print('%s not exists!' % file)
        sys_exit(1)


def cp(src, dst):
    check_file_exists(src)
    debug_print("%-50s \t->\t %s" % (src, dst))
    shutil.copy(src, dst)


# 读取平台信息
def read_platform_info():
    global platformf
    global work_dir
    global img_name
    global pack_tools_dir
    global out_dir

    platformf['chip'] = 'sun20iw1p1'
    platformf['platform'] = 'melis'
    platformf['board_platform'] = 'none'
    platformf['board'] = 'd1-evb-board'
    platformf['debug'] = 'none'
    platformf['sigmode'] = 'none'
    platformf['securemode'] = 'none'
    platformf['mode'] = 'normal'
    platformf['programmer'] = 'none'
    platformf['tar_image'] = 'none'
    # platformf['nor_volume'] = '8'
    platformf['torage_type'] = 'nor'

    work_dir = os.path.join(out_dir, "image_xxx")
    debug_print("work_dir = %s" % work_dir)
    try:
        if os.path.isdir(work_dir):
            if isWindows():
                cmd = "rd /S /Q " + "\"" + work_dir + "\"" + " >nul"
                debug_print(cmd)
                os.system(cmd)
            else:
                shutil.rmtree(work_dir, ignore_errors=True)
        os.mkdir(work_dir)
        os.chdir(work_dir)
    except Exception as e:
        print(str(e))
        sys_exit(1)
    debug_print("goto work_dir")

    img_name = platformf['platform'] + "_" + \
        platformf['board'] + "_" + platformf['debug']

    if not platformf['sigmode'] == "none":
        img_name += "_" + platformf['sigmode']
    if platformf['mode'] == "dump":
        img_name += "_" + platformf['mode']
    if platformf['securemode'] == "secure":
        img_name += "_" + platformf['securemode']
    if platformf['torage_type'] == 'nor':
        img_name += "_" + platformf['nor_volume'] + "Mnor"

    img_name += ".img"
    debug_print("img_name = ", img_name)

    sys.path.append(pack_tools_dir)


def copy_pack_tools():
    global pack_tools_dir
    global work_dir

    image_dir = work_dir

    for file in glob.glob(pack_tools_dir + "/*"):
        dst_file = os.path.join(image_dir, os.path.basename(file))
        shutil.copy(file, dst_file)


def remove_pack_tools():
    global pack_tools_dir
    global work_dir

    image_dir = work_dir + "/"
    for file in glob.glob(pack_tools_dir + "/*"):
        file_name = image_dir + os.path.basename(file)
        # if isWindows():
        #     cmd = "del /F /Q /S " + "\"" + file_name + "\"" + " >nul"
        #     os.system(cmd)
        # else:
        os.remove(file_name)


def copy_commond_files(files, src_dir, dst_dir):
    for item in files:
        src = os.path.join(src_dir, item)
        dst = os.path.join(dst_dir, item)
        if os.path.exists(src):
            shutil.copy(src, dst)

def copy_specital_files(id, files, src_dir, dst_dir):
    if id not in files.keys():
        print(id + "Not support!!!")
        sys_exit(1)
    for item in files[id]:
        src = os.path.join(src_dir, item)
        dst = os.path.join(dst_dir, item)
        if os.path.exists(src):
            shutil.copy(src, dst)


def copy_files():
    global rtos_origin_img
    global fs_img
    global pack_top_dir
    global work_dir
    global mk_generated_imgs_path

    image_dir = work_dir

    config_file_dir = os.path.join(pack_top_dir, "projects", "configs")
    version_file_dir = os.path.join(pack_top_dir, "projects", "version")
    boot_file_dir = os.path.join(pack_top_dir, "projects", "bin")
    epos_file = rtos_origin_img

    debug_print("\n\n----------------------------copy phoenixplugin fex file------------------------------")
    for item in phoenixplugin_file_list:
        for file in glob.glob(pack_top_dir + "/" + item):
            name = os.path.basename(file)
            dst = os.path.join(work_dir, name)
            shutil.copyfile(file, dst)

    debug_print("\n\n----------------------------copy config fex file------------------------------")
    copy_commond_files(common_config_file_list, config_file_dir, image_dir)
    copy_specital_files(platformf['torage_type'], special_fex_config_file, config_file_dir, image_dir)

    debug_print("\n\n----------------------------copy config cfg file------------------------------")
    copy_specital_files(platformf['torage_type'], special_cfg_config_file, config_file_dir, image_dir)

    debug_print("\n\n----------------------------copy version mk file------------------------------")
    copy_commond_files(vesion_file_list, version_file_dir, image_dir)
    debug_print("\n\n----------------------------copy boot bin file------------------------------")

    copy_file = os.path.join(boot_file_dir, "fes1_" + platformf["chip"] + ".bin")
    dst = os.path.join(image_dir, "fes1.fex")
    shutil.copy(copy_file, dst)

    dst = os.path.join(image_dir, "epos.img")
    src = epos_file
    shutil.copy(src, dst)

    if platformf["torage_type"] == "card_product":
        copy_file = os.path.join(boot_file_dir, "boot0_" + platformf["chip"] + "_card.bin")
        dst = os.path.join(image_dir, "boot0_card_product.fex")
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)

    elif platformf["torage_type"] == "nor":
        # copy_file = boot_file_dir + "boot0_" + platformf["chip"] + "_nor.bin"
        dst = os.path.join(image_dir, "boot0_nor.bin")
        src = os.path.join(mk_generated_imgs_path, "data", "boot0")
        shutil.copy(src, dst)

        dst = os.path.join(image_dir, "boot0_nor.fex")
        src = os.path.join(mk_generated_imgs_path, "data", "boot0")
        shutil.copy(src, dst)
        dst = os.path.join(image_dir, "boot_nor.fex")
        src = os.path.join(mk_generated_imgs_path, "data", "boot")
        shutil.copy(src, dst)

        copy_file = os.path.join(boot_file_dir, "u-boot_" + platformf["chip"] + "_nor")
        if not platformf["debug"] and platformf["debug"] != 'none':
            copy_file += "_" + platformf["debug"]
        copy_file += ".bin"
        dst = os.path.join(image_dir, "u-boot_nor.fex")
        shutil.copy(copy_file, dst)
    else:
        copy_file = os.path.join(boot_file_dir, "boot0_" + platformf["chip"] + "_nand.bin")
        dst = os.path.join(image_dir, "boot0_nand.fex")
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)

        copy_file = os.path.join(boot_file_dir, "u-boot_" + platformf["chip"] + "_nand.bin")
        dst = os.path.join(image_dir, "u-boot_nand.fex")
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)
    # boot0_card.fex must copy in order to support card burn
    copy_file = os.path.join(boot_file_dir, "boot0_" + platformf["chip"] + "_card.bin")
    dst = os.path.join(image_dir, "boot0_card.fex")
    shutil.copy(copy_file, dst)

    if platformf["securemode"] == "secure":
        debug_print("\n\n----------------------------copy secure boot file------------------------------")
        copy_file = os.path.join(boot_file_dir, "sboot_" + platformf["chip"] + ".bin")
        dst = os.path.join(image_dir, "sboot.bin")
        src = os.path.relpath(copy_file)
        dst = os.path.relpath(dst)
        cp(src, dst)

    if os.path.exists(fs_img):
        dst = os.path.join(image_dir, "lfs.fex")
        shutil.copy(fs_img, dst)

def del_match(target, match):
    lines = None

    with codecs.open(target, 'r', encoding='utf8') as f:
        lines = f.readlines()
    with codecs.open(target, 'w') as f:
        for l in lines:
            if re.match(match, l):
                continue
            f.write(l)


def set_img_name():
    global img_name
    image_cfg_file = os.path.join(work_dir, "image_" + platformf["torage_type"] + ".cfg")

    lines = None

    with codecs.open(image_cfg_file, 'r') as f:
        lines = f.readlines()
    with codecs.open(image_cfg_file, 'w') as f:
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

    with codecs.open(file, 'rb+') as f1:
        byt = f1.read()

        byt = byt.replace(src, dst)
        tempfile = open(file + toformat, 'wb+')
        tempfile.write(byt)
        tempfile.close()

    os.remove(file)
    os.rename(file + toformat, file)


def copy_image_dir_file(src, dst):
    global work_dir

    src = os.path.join(work_dir, src)
    dst = os.path.join(work_dir, dst)

    src = os.path.relpath(src)
    dst = os.path.relpath(dst)
    if src != dst:
        cp(src, dst)


def mv_image_dir_file(src, dst):
    global work_dir

    if src != dst:
        debug_print("%s -mv-> %s" % (src, dst))
        shutil.move(src, dst)


def exec_cmd(cmd, *args):
    global pack_tools_dir
    global work_dir
    global debug_en

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
        if isWindows():
            cmd_line += " >/nul"
        else:
            cmd_line += " >/dev/null"

    debug_print(cmd_line + "\n")

    if not os.path.exists(exec_file):
        print("!!!exec file %s not exits!!!" % (cmd))
        print("cmd line = %s" % cmd_line)
        sys_exit(1)

    proc = subprocess.Popen(cmd_line, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, shell=True)
    outinfo, errinfo = proc.communicate()
    info = str(outinfo)
    if 'error' in info or 'failed' in info:
        print("exec %s error!" % cmd_line)
        print(info)
        sys_exit(1)


# convert the fex file to bin file
def script_file(storage):
    global pack_tools_dir
    global work_dir

    image_dir = work_dir

    # prepare source files
    config_file_src = "sys_config_" + storage + ".fex"
    config_file_dst = "sys_config_" + storage + ".bin"

    del_match(os.path.join(image_dir, config_file_src), '(^;.*)|(^ +;.*)|(^$)')

    check_file_exists(config_file_src)
    if isWindows():
        exec_cmd("mode_script.exe", config_file_dst, config_file_src)
    else:
        cmd = "busybox unix2dos %s  " % config_file_src
        os.system(cmd)
        exec_cmd("./script", config_file_src)

    cp(os.path.join(image_dir, config_file_dst), os.path.join(image_dir, "config_nor.fex"))
    cp(os.path.join(image_dir, config_file_dst), os.path.join(image_dir, "sys_config.bin"))

    debug_print("script files finish")

def script_partition_file(storage):
    sys_partition_src = "sys_partition_" + storage + ".fex"
    sys_partition_dst = "sys_partition_" + storage + ".bin"
    check_file_exists(sys_partition_src)
    if isWindows():
        exec_cmd("mode_script.exe", sys_partition_dst, sys_partition_src)
    else:
        cmd = "busybox unix2dos %s " % sys_partition_src
        os.system(cmd)
        exec_cmd("./script", sys_partition_src)

def update_boot0(storage='nor'):
    global work_dir

    image_dir = work_dir
    config_file = "sys_config_" + storage + ".bin"

    output_file = image_dir
    storage_type = "SPINOR_FLASH"

    if storage == "nor":
        output_file = os.path.join(output_file, "boot0_nor.fex ")
        storage_type = "SPINOR_FLASH"
    elif storage == "nand":
        output_file = os.path.join(output_file, "boot0_nand.fex ")
        storage_type = "NAND "
    elif storage == "card":
        output_file = os.path.join(output_file, "boot0_card.fex ")
        storage_type = "SDMMC_CARD"
    else:
        output_file = os.path.join(output_file, "boot0_card_product.fex ")
        storage_type = "SDMMC_CARD"

    input_file = os.path.join(image_dir, config_file)

    check_file_exists(input_file)
    if isWindows():
        exec_cmd("update_boot0.exe", output_file, input_file, storage_type)
    else:
        exec_cmd("./update_boot0", output_file, input_file, storage_type)
    # FIXME: just for test
    # print(work_dir)
    # other_boot0_img = os.path.join(work_dir, "..\\image_xxx\\boot0_nor.fex")
    # print(other_boot0_img)
    # check_file_exists(other_boot0_img)
    # shutil.copy(other_boot0_img, "boot0_nor.fex")


def package_boot0(storage):
    global work_dir

    image_dir = work_dir

    src = os.path.join(image_dir, "package_boot0.cfg")
    dst = "melis_pkg_" + storage + ".fex"
    if not os.path.exists(src):
        print("package_boot0.cfg file not exits")
        sys_exit(1)
    check_file_exists("epos.img")
    if isWindows():
        exec_cmd("compress.exe", "-c", "epos.img", "epos.img.lzma")
        exec_cmd("lz4.exe", "epos.img", "epos-lz4.img")
    else:
        exec_cmd("./lz4", "-f", "epos.img", "epos-lz4.img")
        exec_cmd("/usr/bin/lzma", "-zfkv", "epos.img")
    check_file_exists(src)
    if isWindows():
        exec_cmd("dragonsecboot.exe", "-pack", src)
    else:
        exec_cmd("./dragonsecboot", "-pack", src)
    check_file_exists("boot_package.fex")
    mv_image_dir_file("boot_package.fex", dst)


def update_fes1(storage):
    global work_dir

    image_dir = work_dir
    config_file = "sys_config_" + storage + ".bin"

    src = os.path.join(image_dir, config_file)
    dst = os.path.join(image_dir, "fes1.fex")
    if not os.path.exists(src):
        print("sys_config.fex file not exits")
        return
    if isWindows():
        exec_cmd("update_fes1.exe", dst, src)
    else:
        exec_cmd("./update_fes1", dst, src)


def update_uboot(storage):
    global work_dir

    image_dir = work_dir
    config_file = "sys_config_" + storage + ".bin"

    src = os.path.join(image_dir, config_file)
    dst = os.path.join(image_dir, "u-boot_" + storage + ".fex")
    if not os.path.exists(src):
        print("sys_config.fex file not exits")
        return

    check_file_exists(src)
    if isWindows():
        exec_cmd("update_uboot.exe", "-no_merge", dst, src)
    else:
        exec_cmd("./update_uboot", "-no_merge", dst, src)


def package_uboot(storage):
    global work_dir

    image_dir = work_dir
    file_name = "package_uboot_" + storage + ".cfg"
    src = os.path.join(image_dir, file_name)
    dst = "boot_pkg_uboot_" + storage + ".fex"
    temp = os.path.join(image_dir, "boot_package.fex")
    config_file = os.path.join(image_dir, "sys_config_" + storage + ".bin")

    if not os.path.exists(src):
        print("package_boot0.cfg file not exits")
        return

    check_file_exists(src)
    if isWindows():
        exec_cmd("dragonsecboot.exe", "-pack", src)
    else:
        exec_cmd("./dragonsecboot", "-pack", src)

    check_file_exists(temp)
    if isWindows():
        exec_cmd("update_toc1.exe", temp, config_file)
    else:
        exec_cmd("./update_toc1", temp, config_file)
    mv_image_dir_file(temp, dst)


def update_env():
    global work_dir

    image_dir = work_dir

    src = os.path.join(image_dir, "env.cfg")
    dst = os.path.join(image_dir, "env.fex")

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
    with codecs.open(file, 'r') as f:
        lines = f.readlines()
    with codecs.open(tempfile, 'w') as f:
        for l in lines:
            if re.match("(^;.*)|(^ +;.*)|(^$)", l):
                continue
            if start:
                f.write(l)
            if "[partition_start]" in l:
                start = True
    format_file(tempfile, "dos2unix")
    with codecs.open(tempfile, 'r') as f:
        lines = f.readlines()
    part_str = ""
    for l in lines:
        if "[partition]" in l:
            part_str += '~'
        else:
            part_str += l
    for item in re.split('~', part_str):
        if item != "" and item != '\n':
            part_info = re.split('=|\n+', item)
            del part_info[-1]
            for i in range(len(part_info)):
                part_info[i] = part_info[i].strip()

            if len(part_info) % 2 != 0:
                print("parse part info failed0!")
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
                print("parse part info failed1!")
                return None
    for k, v in info.items():
        if k == 'UDISK':
            v['size'] = str(total - sum)
            _udisk_size = (total - sum) * 1024 / 512

    return info, _udisk_size


def perpare_for_nor():
    global work_dir
    global udisk_size

    image_dir = work_dir

    partition_file = os.path.join(image_dir, "sys_partition_nor.fex")
    part_kb_for_nor = int(platformf['nor_volume'])*1024 - 64

    part_info, udisk_size = get_part_info(partition_file, part_kb_for_nor)
    debug_print(part_info)
    # # make fs
    # name = top + "\\projects\\" + platformf['board'] + "\\data\\UDISK"
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

    image_dir = work_dir

    sys_partition_file = os.path.join(image_dir, "sys_partition_" + storage + ".bin")
    dlinfo_file = os.path.join(image_dir, "dlinfo.fex")
    gpt_file = os.path.join(image_dir, "sunxi_gpt.fex")
    mbr_source_file = os.path.join(image_dir, "sunxi_mbr_" + storage + ".fex")
    boot0_file_name = os.path.join(image_dir, "boot0_.fex")

    # update mbr file
    check_file_exists(sys_partition_file)
    if storage == 'nor':
        if isWindows():
            exec_cmd("update_mbr.exe",  sys_partition_file, mbr_source_file, dlinfo_file, 1)
        else:
            exec_cmd("./update_mbr",  sys_partition_file, 1, mbr_source_file)
    else:
        if isWindows():
            exec_cmd("update_mbr.exe",  sys_partition_file, mbr_source_file, dlinfo_file, 4)
        else:
            exec_cmd("./update_mbr",  sys_partition_file, mbr_source_file, dlinfo_file, 4)
    # convert mbr to get
    debug_print("----------------mbr convert to gpt start---------------------")
    check_file_exists(mbr_source_file)
    if isWindows():
        exec_cmd("convert_gpt.exe", "-s",  mbr_source_file, "-o", gpt_file, "-l 96", "-u", platformf['nor_volume'])
    else:
        exec_cmd("./mbr_convert_to_gpt", "-s",  mbr_source_file, "-o", gpt_file, "-l 96", "-u", platformf['nor_volume'])
    debug_print("update mbr finish")


def create_img(part_info):
    global work_dir
    global platformf
    global img_name
    global factory_zip

    debug_print("\n\n----------------------------create image------------------------------")

    storage = platformf['torage_type']
    image_dir = work_dir

    debug_print("image name = %s" % img_name)
    image_src = img_name

    img_cfg_file = os.path.join(image_dir, "image_" + storage + ".cfg")
    sys_partition_file = os.path.join(image_dir, "sys_partition_" + storage + ".fex")

    # check file exists
    check_file_exists(img_cfg_file)
    check_file_exists(sys_partition_file)

    format_file(img_cfg_file, 'dos2unix')
    with codecs.open(img_cfg_file, 'r') as f:
        for line in f:
            if re.search('filename[ ]+=[ ]".*"', line):
                filename = re.sub('"', '', line.split(',')[
                                  0].split('=')[1]).strip()
                filepath = os.path.join(os.path.relpath(image_dir), filename)
                if not os.path.exists(filepath):
                    print("%-30s <Failed>" % filename)
                else:
                    debug_print("%-30s <Success>" % filename)
    if part_info == None:
        print('partition error!')
        sys_exit(1)
    for k, v in part_info.items():
        if 'downloadfile' in v.keys():
            filename = v['downloadfile']
            filename = re.sub('"', '', filename)
            filepath = os.path.join(os.path.relpath(image_dir), filename)
            if not os.path.exists(filepath):
                print("%-30s <Failed>" % filename)
            else:
                debug_print("DL::%-30s <Success>" % filename)
    if isWindows():
        exec_cmd("dragon.exe",  img_cfg_file, sys_partition_file)
    else:
        # exec_cmd("./dragon",  img_cfg_file, sys_partition_file)
        cmd = "./dragon %s %s >/dev/null" % (img_cfg_file, sys_partition_file)
        ret = os.system(cmd)
        if ret:
            print("Please try to excute `sudo service binfmt-support start` first.")

    if not os.path.exists(image_src):
        print("Create image failed!")
        sys_exit(1)
    final_image_name = "yoc_rtos_" + platformf['nor_volume'] + "M.img"
    if factory_zip and os.path.isfile(factory_zip):
        final_image_name = "yoc_rtos_" + (os.path.basename(factory_zip)).split('.')[0] + "_" + platformf['nor_volume'] + "M.img"
    image_dst = os.path.join(out_dir, final_image_name)
    if os.path.exists(image_dst):
        if isWindows():
            cmd = "del /F /Q /S " + "\"" + image_dst + "\"" + " >nul"
            os.system(cmd)
        else:
            os.remove(image_dst)
    if isWindows():
        cmd = "move /Y " + "\"" + image_src + "\" \"" + image_dst + "\"" + " >nul"
        os.system(cmd)
    else:
        shutil.move(image_src, image_dst)
    print('Create %s in out directory Success!' % final_image_name)


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

    updarte_mbr(storage, udisk_size, platformf['nor_volume'])
    create_img(part_info)

def variables_init():
    global debug_en
    global rtos_origin_img
    global fs_data_dir
    global out_dir
    global mk_generated_imgs_path
    global product_file
    global pack_top_dir
    global pack_tools_dir
    global configs_dir
    global config_yaml
    global factory_zip

    debug_en = iargs.debug_en
    rtos_origin_img = iargs.rtos_origin_img
    fs_data_dir = iargs.fs_data_dir
    out_dir = iargs.out_dir
    mk_generated_imgs_path = iargs.mk_generated_imgs_path
    config_yaml = iargs.config_yaml
    factory_zip = iargs.factory_zip

    debug_print(debug_en)
    debug_print(rtos_origin_img)
    debug_print(fs_data_dir)
    debug_print(out_dir)
    debug_print(mk_generated_imgs_path)
    debug_print(config_yaml)
    debug_print(factory_zip)

    if getattr(sys, 'frozen', False):
        pack_top_dir = os.path.dirname(os.path.realpath(sys.executable))
    elif __file__:
        pack_top_dir = os.path.split(os.path.realpath(__file__))[0]
    if isWindows():
        pack_tools_dir = os.path.join(pack_top_dir, "tools/packtool-win")
    else:
        pack_tools_dir = os.path.join(pack_top_dir, "tools/packtool")
    debug_print("pack_top_dir: %s" % pack_top_dir)
    debug_print("pack_tools_dir: %s" % pack_tools_dir)
    configs_dir = os.path.join(pack_top_dir, "../configs")
    debug_print("configs_dir: %s" % configs_dir)
    if isWindows():
        product_file = os.path.join(configs_dir, "product.exe")
    else:
        product_file = os.path.join(configs_dir, "product")
    if not os.path.isdir(pack_top_dir):
        print("the pack dir is not exist")
        sys_exit(0)
    if not rtos_origin_img or not os.path.isfile(rtos_origin_img):
        print("the origin rtos image file is not exist.[%s]" % rtos_origin_img)
        sys_exit(0)
    if not product_file or not os.path.isfile(product_file):
        print("the product.exe is not exist.")
        sys_exit(0)
    if not out_dir or not os.path.isdir(out_dir):
        print("the out dir is not exist.")
        sys_exit(0)
    if not mk_generated_imgs_path or not os.path.isdir(mk_generated_imgs_path):
        print("the generated dir is not exist.")
        sys_exit(0)
    if not os.path.isdir(configs_dir):
        print("the configs dir is not exist.")
        sys_exit(0)
    config_yaml = os.path.abspath(config_yaml)
    if not config_yaml or not os.path.isfile(config_yaml):
        print("the config.yaml file not exist.")
        sys_exit(0)
    if os.path.isdir(fs_data_dir):
        fs_data_dir = os.path.abspath(fs_data_dir)
    debug_print("variable init ok.")

def do_imageszip_prepare():
    global mk_generated_imgs_path
    global work_dir

    debug_print("do imageszip prepare start.")
    check_file_exists("melis_pkg_nor.fex")
    check_file_exists("boot0_nor.fex")
    shutil.copy("melis_pkg_nor.fex", os.path.join(mk_generated_imgs_path, "data", "prim"))
    shutil.copy("boot0_nor.fex", os.path.join(mk_generated_imgs_path, "data", "boot0"))
    if os.path.exists("lfs.fex"):
        shutil.copy("lfs.fex", os.path.join(mk_generated_imgs_path, "data", "lfs"))

    data_dir = os.path.join(mk_generated_imgs_path, "data")
    for file in glob.glob(data_dir + "/*"):
        dst_file = os.path.join(work_dir, os.path.basename(file) + ".fex")
        shutil.copy(file, dst_file)

    # copy a temporary imtb.fex just for update mbr
    config_file_dir = os.path.join(pack_top_dir, "projects", "configs")
    check_file_exists(os.path.join(config_file_dir, "iii.fex"))
    dst_file = os.path.join(work_dir, "imtb.fex")
    shutil.copy(os.path.join(config_file_dir, "iii.fex"), dst_file)

    debug_print("do imageszip prepare finish.")

def do_imageszip():
    global mk_generated_imgs_path
    global product_file
    global work_dir

    debug_print("create images.zip start.")
    check_file_exists(product_file)
    images_zip = os.path.join(mk_generated_imgs_path, "images.zip")
    if os.path.exists("sunxi_gpt.fex"):
        shutil.copy("sunxi_gpt.fex", os.path.join(mk_generated_imgs_path, "data", "gpt"))
    cmd = "".join("\"%s\" %s %s %s %s %s " % (product_file, "image", images_zip, "-i", os.path.join(mk_generated_imgs_path, "data"), "-l -p"))
    os.system(cmd)
    cmd = "".join("\"%s\" %s %s %s %s %s " % (product_file, "image", images_zip, "-e", mk_generated_imgs_path, "-x"))
    os.system(cmd)
    bin_dir = os.path.join(mk_generated_imgs_path, "bin")
    cmd = "".join("\"%s\" %s %s %s %s " % (product_file, "image", images_zip, "-e", bin_dir))
    os.system(cmd)

    # update the fex firmware
    check_file_exists(os.path.join(bin_dir, "prim"))
    check_file_exists(os.path.join(bin_dir, "imtb"))
    for file in glob.glob(bin_dir + "/*"):
        dst_file = os.path.join(work_dir, os.path.basename(file) + ".fex")
        shutil.copy(file, dst_file)

    debug_print("create images.zip finish.")

def do_factory_zip():
    global mk_generated_imgs_path
    global out_dir
    global work_dir
    global factory_zip

    debug_print("do %s start." % factory_zip)
    tmpfac = os.path.join(work_dir, ".factory")
    check_file_exists(factory_zip)
    if os.path.exists(tmpfac):
        exec_cmd("rm", "-rf", tmpfac)
        exec_cmd("mkdir", "-p", tmpfac)
    if os.path.isdir(tmpfac):
        print("The dir %s is not found." % tmpfac)
        sys_exit(1)
    with zipfile.ZipFile(factory_zip) as zf:
        zf.extractall(tmpfac)
    check_file_exists(os.path.join(tmpfac, "prim"))
    check_file_exists(os.path.join(tmpfac, "imtb"))
    check_file_exists(os.path.join(tmpfac, "boot"))
    for file in glob.glob(tmpfac + "/*"):
        if os.path.isfile(file):
            filename = os.path.basename(file)
            if filename != 'config.yaml' and filename != 'total_image.bin':
                dst_file = os.path.join(work_dir, filename + ".fex")
                debug_print(dst_file)
                shutil.copy(file, dst_file)
    debug_print("do %s finish." % factory_zip)

def do_yoc_pack():
    global factory_zip

    if factory_zip and os.path.isfile(factory_zip):
        do_factory_zip()
        return True
    else:
        do_imageszip_prepare()
        return False

def generate_sys_partition_file(partitions):
    global work_dir

    def gen_partition_line(parts):
        text = ''''''
        for p in parts:
            name = p['name']
            if name == 'boot0' or name == 'gpt':
                continue
            filename = name
            if 'file' in p:
                filename = p['file']

            size = int(p['size'])
            filepath = os.path.join(mk_generated_imgs_path, "data", filename)
            if os.path.isfile(filepath) or name == 'imtb' or name == 'bmtb':
                if size % 512 != 0:
                    print("The [%s] partition size must be multiple of 512." % name)
                    print('''Please check the "%s" file.''' % config_yaml)
                    sys_exit(1)
                if (name != 'imtb' and name != 'bmtb') and os.path.getsize(filepath) > size:
                    print("The [%s] partition's file is overflow." % name)
                    print('''Please check the "%s" file.''' % config_yaml)
                    sys_exit(1)
                filename +=  '.fex'
                text += '''
[partition]
    name         = %s
    size         = %d
    downloadfile = "%s"
    user_type    = 0x8000''' % (name, size/512, filename)
            else:
                text += '''
[partition]
    name         = %s
    size         = %d
    user_type    = 0x8000''' % (name, size/512)
        return text

    text = '''

[mbr]
size = 16

[partition_start]
%s
''' % gen_partition_line(partitions)

    filepath = os.path.join(work_dir , "sys_partition_nor.fex")
    with codecs.open(filepath, "w") as f:
        f.write(text)
    format_file(filepath, "dos2unix")
    debug_print("generate sys_partition_nor.fex ok")

def generate_little_fs():
    global platformf
    global configs_dir
    global work_dir
    global fs_data_dir
    global fs_img

    fs_size = platformf['fs_size']
    mklittlefs = "mklittlefs_linux"
    fs_img = os.path.join(work_dir, "littlefs.bin")
    if os.path.exists(fs_data_dir):
        if isWindows():
            mklittlefs = "mklittlefs.exe"
        exec_cmd(os.path.join(configs_dir, mklittlefs),  "-c", fs_data_dir,  "-d 5 -b 4096 -p 1792", "-s", int(fs_size), fs_img)
        if not os.path.isfile(fs_img):
            print("littlefs.bin genetared failed.")
            sys_exit(0)


def parser_config_yaml():
    global platformf
    global config_yaml

    platformf['fs_size'] = int(0)
    platformf['nor_volume'] = '8'

    debug_print('parser config yaml')
    try:
        conf = Package(config_yaml)
        flash = conf.flash
        platformf['nor_volume'] = "%d" % (int(flash['size'])/1024/1024)
        partitions = conf.partitions
        if isinstance(partitions, list):
            for ele in partitions:
                if "lfs" == ele['name'] or "littlefs" == ele['name']:
                    platformf['fs_size'] = int(ele['size'])
                    return partitions
        print("parser config.yaml error or there is no lfs partition.")
    except Exception as e:
        print(str(e))

    sys_exit(1)


if __name__ == "__main__":
    part_info = None
    variables_init()
    partitions = parser_config_yaml()
    read_platform_info()
    generate_little_fs()
    copy_files()
    copy_pack_tools()
    storage = platformf['torage_type']
    do_update(storage)
    is_factory = do_yoc_pack()
    generate_sys_partition_file(partitions)
    script_partition_file(storage)
    if storage == "nor":
        part_info = perpare_for_nor()
    updarte_mbr(storage, udisk_size, platformf['nor_volume'])
    if not is_factory:
        do_imageszip()
    create_img(part_info)
    remove_pack_tools()
