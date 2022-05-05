#!/usr/bin/python
import os
import sys
import re
import csv
import xlsxwriter
import operator

# Installing XlsxWriter:
# $ pip install XlsxWriter
# Or to a non system dir:
# $ pip install --user XlsxWriter

#compiler name
compiler_name = ''
#data compress
data_compress = ''

#excel formats
format_dict_title = {'bold': True, 'border': 7, 'align': 'left', 'bg_color': '#FFE0E0'}
format_dict_entry = {'border': 7, 'align': 'left', 'bg_color': '#E0FFE0'}
format_dict_entry_A = {'border': 7, 'align': 'left', 'bg_color': '#40FF40'}
format_dict_entry_B = {'border': 7, 'align': 'left', 'bg_color': '#80FF80'}
format_dict_entry_C = {'border': 7, 'align': 'left', 'bg_color': '#C0FFC0'}

#excel formats '.a', '.a', '.a', '.a', '.a', '.a', '.a', '.a', '.a', '.a',
#base on alios-things release rel_2.1
libname_3rd = ['littlevGL.a', 'micropython.a']
libname_sec = ['alicrypto.a', 'mbedtls.a', 'imbedtls.a', 'dpm.a', 'id2.a', 'irot.a', 'libkm.a', 'libplat_gen.a', 'se.a', 'libkm_tee.a', 'libsst.a', 'itls.a', 'libprov.a', ]
libname_uti = ['cli.a', 'activation.a', 'chip_code.a', 'base64.a', 'cjson.a', 'newlib_stub.a', 'zlib.a']
libname_fs  = ['cramfs.a', 'fatfs.a', 'jffs2.a', 'spiffs.a', 'uffs.a', 'yaffs2.a', 'ramfs.a', 'vfs.a']
libname_knl = ['helloworld.a', 'mbmaster.a', 'usb.a', 'cplusplus.a', 'debug.a', 'kv.a', 'kernel_init.a', 'kmbins.a', 'umbins.a', 'mm.a', 'pwrmgmt.a', 'rhino.a', 'uspace.a', 'osal.a', 'cmsis.a', 'posix.a']
libname_arc = ['arch_armv5.a', 'arch_armv6m.a', 'arch_armv7a.a', 'arch_armv7m.a', 'arch_linux.a', 'arch_mips32.a', 'arch_risc_v32I.a', 'arch_xtensa_lx106.a', 'arch_xtensa_lx6.a']
libname_net = ['bleadv.a', 'blemesh_tmall.a', 'breezeapp.a', 'ble.a', 'comboapp.a', 'breeze.a', 'breeze_hal.a', 'bt.a', 'bt_host.a', 'bt_common.a', 'bt_profile.a', 'bt_mesh.a', 'port.a', 'tmall_model.a', 'lorawan_4_4_0.a', 'lorawan_4_4_2.a', 'lwip.a', 'athost.a', 'atparser.a', 'netmgr.a', 'umesh.a', 'yloop.a']
libname_mid = ['otaapp.a', 'tiny_engine.a', 'ulog.a', 'ota.a', 'ota_2nd_boot.a', 'ota_hal.a', 'ota_ble.a', 'ota_core.a', 'udata.a']
libname_wss = ['libawss.a']
libname_lkt = ['linkkitapp.a', 'linkkit_gateway.a', 'linkkit_sdk_c.a', 'libiot_log.a', 'libiot_system.a', 'libiot_utils.a', 'libiot_alcs.a', 'libiot_coap_cloud.a', 'libiot_coap_coappack.a', 'libiot_coap_local.a', 'libiot_http.a', 'libiot_http2.a', 'libiot_mqtt.a', 'iotx-hal.a', 'libiot_sdk_impl.a', 'libdev_bind.a', 'libiot_http2_stream.a', 'libiot_cm.a', 'libdev_reset.a', 'libiot_dm.a', 'libiot_mal.a', 'sal.a', ]
libname_std = ['libm.a', 'libgcc.a', 'libc_nano.a', 'libc.a', 'libcirom.a']

#add modules for earlier alios-things release
libname_uti += ['common.a', 'digest_algorithm.a', 'framework.a']
libname_mid += ['log.a', 'uOTA.a', 'download_http.a', 'fota.a', 'fota_download.a', 'fota_mqtt_transport.a', 'fota_platform.a', 'socket_stand.a', 'ota_download.a', 'ota_service.a', 'ota_transport.a', 'ota_verify.a']
libname_knl += ['vcall.a']
libname_arc += ['xtensa.a', 'armv5.a', 'armv6m.a', 'armv7a.a', 'armv7m.a', 'linux.a', 'mips32.a', 'risc_v32I.a', 'xtensa_lx106.a', 'xtensa_lx6.a']
libname_lkt += ['MQTTPacket.a', 'alcs.a', 'cm.a', 'dm.a', 'iotkit.a', 'link-coap.a', 'mqtt.a', 'libiot_sdk.a', '.a', '.a']
libname_wss += ['libywss.a']
libname_net += ['net.a']

#library (*.a) parse
def find_lib_owner(libname):
    if libname in libname_sec:
        return 'security', format_dict_entry_B
    elif libname in libname_uti:
        return 'utility', format_dict_entry_B
    elif libname in libname_fs:
        return 'filesystem', format_dict_entry_B
    elif libname in libname_knl:
        return 'kernel', format_dict_entry_B
    elif libname in libname_arc:
        return 'kernel', format_dict_entry_B
    elif libname in libname_net:
        return 'network', format_dict_entry_B
    elif libname in libname_mid:
        return 'middleware', format_dict_entry_B
    elif libname in libname_wss:
        return 'awss', format_dict_entry_B
    elif libname in libname_lkt:
        return 'linkkit', format_dict_entry_B
    elif libname in libname_3rd:
        return '3rdpart', format_dict_entry_C
    elif libname in libname_std:
        return 'standlib', format_dict_entry_C
    else :
        return 'bsp', format_dict_entry_A

#get symbol list from gcc map file
def get_sym_list_gcc(sym_all_list, map_file, mem_map_text):
    #1. get 'mem_map_text'
    # find memory map (without discard and debug sections)
    mem_map_list = re.findall(r'Linker script and memory map([\s\S]+?)OUTPUT', mem_map_text)
    mem_map_text = '' if not mem_map_list else mem_map_list[0]
    if not mem_map_text:
        print ('Can\'t parse memory info, memory info get fail!')
        return
    mem_map_text = mem_map_text.replace('\r', '')

    #2. find all object file (*.o) map info
    sym_all_list_a = re.findall(r' [\.\w]*\.(iram1|text|literal|rodata|rodata1|data|bss)(?:\.(\S+)\n? +| +)(0x\w+) +(0x\w+) +.+[/\\](.+\.a)\((.+\.o)\)\n', mem_map_text)
    sym_all_list_a = map(lambda arg : {'Type':arg[0], 'Sym':arg[1], 'Addr':int(arg[2], 16),
                     'Size':int(arg[3], 16), 'Lib':arg[4], 'File':arg[5]}, sym_all_list_a)

    sym_all_list_o = re.findall(r' [\.\w]*\.(iram1|text|literal|rodata|data|bss|mmu_tbl)(?:\.(\S+)\n? +| +)(0x\w+) +(0x\w+) +.+[/\\](.+\.o)\n', mem_map_text)
    sym_all_list_o = map(lambda arg : {'Type':arg[0], 'Sym':arg[1], 'Addr':int(arg[2], 16),
                     'Size':int(arg[3], 16), 'Lib':'null', 'File':arg[4]}, sym_all_list_o)

    sym_com_list_a = re.findall(r' (COMMON) +(0x\w+) +(0x\w+) +.+[/\\](.+\.a)\((.+\.o)\)\n +0x\w+ +(\w+)\n', mem_map_text)
    sym_com_list_a = map(lambda arg : {'Type':arg[0], 'Sym':arg[5], 'Addr':int(arg[1], 16),
                     'Size':int(arg[2], 16), 'Lib':arg[3], 'File':arg[4]}, sym_com_list_a)

    sym_com_list_o = re.findall(r' (COMMON) +(0x\w+) +(0x\w+) +.+[/\\](.+\.o)\n +0x\w+ +(\w+)\n', mem_map_text)
    sym_com_list_o = map(lambda arg : {'Type':arg[0], 'Sym':arg[4], 'Addr':int(arg[1], 16),
                     'Size':int(arg[2], 16), 'Lib':'null', 'File':arg[3]}, sym_com_list_o)

    sym_all_list.extend(sym_all_list_a)
    sym_all_list.extend(sym_all_list_o)
    sym_all_list.extend(sym_com_list_a)
    sym_all_list.extend(sym_com_list_o)

#get symbol list from gcc map file
def get_sym_list_armcc(sym_all_list, map_file, mem_map_text):
    #1. get 'mem_map_text'
    mem_map_text = mem_map_text.replace('\r', '')

    #2. find all object file (*.o) map info
    sym_all_list_o = re.findall(r'\s+(0x\w+)\s+(0x\w+)\s+(Zero|Data|Code)\s+(RW|RO)\s+\d+\s+(\S+)\s+(.+\.o)\n', mem_map_text)
    sym_all_list_o = map(lambda arg : {'Addr':arg[0], 'Size':int(arg[1], 16), 'Type':arg[2],
                         'Attr':arg[3], 'Sym':arg[4], 'Lib': 'null', 'File':arg[5]}, sym_all_list_o)
    sym_all_list.extend(sym_all_list_o)

    sym_all_list_a = re.findall(r'\s+(0x\w+)\s+(0x\w+)\s+(Zero|Data|Code)\s+(RW|RO)\s+\d+\s+(\S+)\s+(\w+\.ar?)\((.+\.o)\)\n', mem_map_text)
    sym_all_list_a = map(lambda arg : {'Addr':arg[0], 'Size':int(arg[1], 16), 'Type':arg[2],
                         'Attr':arg[3], 'Sym':arg[4], 'Lib': arg[5], 'File':arg[6]}, sym_all_list_a)
    sym_all_list.extend(sym_all_list_a)

    sym_all_list_l = re.findall(r'\s+(0x\w+)\s+(0x\w+)\s+(Zero|Data|Code)\s+(RW|RO)\s+\d+\s+(\S+)\s+(\w+\.l)\((.+\.o)\)\n', mem_map_text)
    sym_all_list_l = map(lambda arg : {'Addr':arg[0], 'Size':int(arg[1], 16), 'Type':arg[2],
                         'Attr':arg[3], 'Sym':arg[4], 'Lib': arg[5], 'File':arg[6]}, sym_all_list_l)
    sym_all_list.extend(sym_all_list_l)

#library (*.a) parse
def parse_library(sym_all_list, benchbook):
    lib_dic_list = []
    id_list = []

    #for each memmap info, classify by mem type
    for obj_dic in sym_all_list:
        id_str = obj_dic['Lib']
        if id_str not in id_list:
            idx = len(lib_dic_list)
            lib_dic_list.append({'Lib':obj_dic['Lib'], 'RAM':0, 'Text':0, 'Rodata':0, 'Data':0, 'Bss':0})
            id_list.append(id_str)
        else:
            idx = id_list.index(id_str)

        if compiler_name == 'gcc':
            if obj_dic['Type'] == 'text' or obj_dic['Type'] == 'literal' or obj_dic['Type'] == 'iram1':
                lib_dic_list[idx]['Text'] += obj_dic['Size']
            elif obj_dic['Type'] == 'rodata' or obj_dic['Type'] == 'rodata1':
                lib_dic_list[idx]['Rodata'] += obj_dic['Size']
            elif obj_dic['Type'] == 'data':
                lib_dic_list[idx]['Data'] += obj_dic['Size']
            elif obj_dic['Type'] == 'bss' or obj_dic['Type'] == 'COMMON' or obj_dic['Type'] == 'mmu_tbl':
                lib_dic_list[idx]['Bss'] += obj_dic['Size']
        elif compiler_name == 'armcc':
            if obj_dic['Type'] == 'Code':
                lib_dic_list[idx]['Text'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Data' and obj_dic['Attr'] == 'RO':
                lib_dic_list[idx]['Rodata'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Data' and obj_dic['Attr'] == 'RW':
                lib_dic_list[idx]['Data'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Zero':
                lib_dic_list[idx]['Bss'] += obj_dic['Size']

    #sum ROM and RAM for each library file
    for lib_dic in lib_dic_list:
        lib_dic['RAM'] = lib_dic['Text'] + lib_dic['Rodata'] + lib_dic['Data'] + lib_dic['Bss']

    title_format = benchbook.add_format(format_dict_title)

    #2. add obj_dic_list to excel table
    worksheet = benchbook.add_worksheet('Library')
    worksheet.set_column('A:A', 20)
    worksheet.set_column('B:F', 10)
    row = 0

    #set table title
    worksheet.write_row(row, 0, ['MODULE', 'OWNER', 'TOTAL', 'TEXT', 'RODATA', 'DATA', 'Bss'], title_format)
    row += 1

    #add table entry
    lib_dic_list = sorted(lib_dic_list, key=operator.itemgetter('RAM'), reverse=True)
    for lib_dic in lib_dic_list:
        if lib_dic['RAM'] == 0:
            continue
        (lib_owner, format_entry) = find_lib_owner(lib_dic['Lib'])
        entry_format = benchbook.add_format(format_entry)
        worksheet.write_row(row, 0, [lib_dic['Lib'], lib_owner, lib_dic['RAM'], lib_dic['Text'], lib_dic['Rodata'], lib_dic['Data'], lib_dic['Bss']], entry_format)
        row += 1

    #table ending, summary
    worksheet.write_row(row, 0, ['TOTAL (bytes)', ''], title_format)
    worksheet.write_formula(row, 2, '=SUM(C2:C' + str(row) + ')', title_format)
    worksheet.write_formula(row, 3, '=SUM(D2:D' + str(row) + ')', title_format)
    worksheet.write_formula(row, 4, '=SUM(E2:E' + str(row) + ')', title_format)
    if data_compress == 'no':
        worksheet.write_formula(row, 5, '=SUM(F2:F' + str(row) + ')', title_format)
    worksheet.write_formula(row, 6, '=SUM(G2:G' + str(row) + ')', title_format)

#object file (*.o) parse
def parse_object(sym_all_list, benchbook):
    obj_dic_list = []
    id_list = []

    #for each memmap info, classify by mem type
    for obj_dic in sym_all_list:
        id_str = obj_dic['File'] + obj_dic['Lib']
        if id_str not in id_list:
            idx = len(obj_dic_list)
            obj_dic_list.append({'File':obj_dic['File'], 'Lib':obj_dic['Lib'], 'RAM':0, 'Text':0, 'Rodata':0, 'Data':0, 'Bss':0})
            id_list.append(id_str)
        else:
            idx = id_list.index(id_str)

        if compiler_name == 'gcc':
            if obj_dic['Type'] == 'text' or obj_dic['Type'] == 'literal' or obj_dic['Type'] == 'iram1':
                obj_dic_list[idx]['Text'] += obj_dic['Size']
            elif obj_dic['Type'] == 'rodata' or obj_dic['Type'] == 'rodata1':
                obj_dic_list[idx]['Rodata'] += obj_dic['Size']
            elif obj_dic['Type'] == 'data':
                obj_dic_list[idx]['Data'] += obj_dic['Size']
            elif obj_dic['Type'] == 'bss' or obj_dic['Type'] == 'COMMON' or obj_dic['Type'] == 'mmu_tbl':
                obj_dic_list[idx]['Bss'] += obj_dic['Size']
        elif compiler_name == 'armcc':
            if obj_dic['Type'] == 'Code':
                obj_dic_list[idx]['Text'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Data' and obj_dic['Attr'] == 'RO':
                obj_dic_list[idx]['Rodata'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Data' and obj_dic['Attr'] == 'RW':
                obj_dic_list[idx]['Data'] += obj_dic['Size']
            elif obj_dic['Type'] == 'Zero':
                obj_dic_list[idx]['Bss'] += obj_dic['Size']

    #sum ROM and RAM for each objrary file
    for obj_dic in obj_dic_list:
        obj_dic['RAM'] = obj_dic['Text'] + obj_dic['Rodata'] + obj_dic['Data'] + obj_dic['Bss']

    title_format = benchbook.add_format(format_dict_title)

    #2. add obj_dic_list to excel table
    worksheet = benchbook.add_worksheet('Object')
    worksheet.set_column('A:A', 24)
    worksheet.set_column('B:B', 20)
    worksheet.set_column('C:E', 10)
    row = 0

    #set table title
    worksheet.write_row(row, 0, ['C FILE', 'MODULE', 'TOTAL', 'TEXT', 'RODATA', 'DATA'], title_format)
    row += 1

    #add table entry
    obj_dic_list = sorted(obj_dic_list, key=operator.itemgetter('RAM'), reverse=True)
    for obj_dic in obj_dic_list:
        (lib_owner, format_entry) = find_lib_owner(obj_dic['Lib'])

        entry_format = benchbook.add_format(format_entry)
        worksheet.write_row(row, 0, [obj_dic['File'], obj_dic['Lib'], obj_dic['RAM'], obj_dic['Text'], obj_dic['Rodata'], obj_dic['Data']], entry_format)
        row += 1

    #table ending, summary
    worksheet.write_row(row, 0, ['TOTAL (bytes)', '', ''], title_format)
    worksheet.write_formula(row, 2, '=SUM(C2:C' + str(row) + ')', title_format)
    worksheet.write_formula(row, 3, '=SUM(D2:D' + str(row) + ')', title_format)
    worksheet.write_formula(row, 4, '=SUM(E2:E' + str(row) + ')', title_format)
    worksheet.write_formula(row, 5, '=SUM(F2:F' + str(row) + ')', title_format)

#symbol parse
def parse_symbol(sym_all_list, benchbook):
    func_dic_list = []
    rodt_dic_list = []
    vari_dic_list = []
    id_list = []

    #for each memmap info, classify by mem type, add table entry
    for sym_dic in sym_all_list:
        if compiler_name == 'gcc':
            if sym_dic['Type'] == 'text' or sym_dic['Type'] == 'literal' or sym_dic['Type'] == 'iram1':
                func_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})
            elif sym_dic['Type'] == 'rodata':
                rodt_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})
            elif sym_dic['Type'] == 'data' or sym_dic['Type'] == 'bss' or sym_dic['Type'] == 'COMMON':
                vari_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})
        if compiler_name == 'armcc':
            if sym_dic['Type'] == 'Code':
                func_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})
            elif sym_dic['Attr'] == 'RO':
                rodt_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})
            else:
                vari_dic_list.append({'Sym':sym_dic['Sym'], 'File':sym_dic['File'],
                                      'Lib':sym_dic['Lib'], 'Size':sym_dic['Size']})

    title_format = benchbook.add_format(format_dict_title)

    #2. add func_dic_list to excel table
    worksheet = benchbook.add_worksheet('Function')
    worksheet.set_column('A:B', 24)
    worksheet.set_column('C:C', 20)
    worksheet.set_column('D:E', 10)
    row = 0

    worksheet.write_row(row, 0, ['FUNCTION', 'C FILE', 'MODULE', 'OWNER', 'SIZE'], title_format)
    row += 1

    func_dic_list = sorted(func_dic_list, key=operator.itemgetter('Size'), reverse=True)
    for sym_dic in func_dic_list:
        (lib_owner, format_entry) = find_lib_owner(sym_dic['Lib'])
        entry_format = benchbook.add_format(format_entry)
        worksheet.write_row(row , 0, [sym_dic['Sym'], sym_dic['File'],
                            sym_dic['Lib'], lib_owner, sym_dic['Size']], entry_format)
        row += 1

    worksheet.write_row(row, 0, ['TOTAL', '', '', ''], title_format)
    worksheet.write_formula(row, 4, '=SUM(E2:E' + str(row) + ')', title_format)

    #3. add func_dic_list to excel table
    worksheet = benchbook.add_worksheet('Rodata')
    worksheet.set_column('A:B', 24)
    worksheet.set_column('C:C', 20)
    worksheet.set_column('D:E', 10)
    row = 0

    worksheet.write_row(row, 0, ['Rodata', 'C FILE', 'MODULE', 'OWNER', 'SIZE'], title_format)
    row += 1

    rodt_dic_list = sorted(rodt_dic_list, key=operator.itemgetter('Size'), reverse=True)
    for sym_dic in rodt_dic_list:
        (lib_owner, format_entry) = find_lib_owner(sym_dic['Lib'])
        entry_format = benchbook.add_format(format_entry)
        worksheet.write_row(row , 0, [sym_dic['Sym'], sym_dic['File'],
                            sym_dic['Lib'], lib_owner, sym_dic['Size']], entry_format)
        row += 1

    worksheet.write_row(row, 0, ['TOTAL', '', '', ''], title_format)
    worksheet.write_formula(row, 4, '=SUM(E2:E' + str(row) + ')', title_format)

    #4. add func_dic_list to excel table
    worksheet = benchbook.add_worksheet('Variable')
    worksheet.set_column('A:B', 24)
    worksheet.set_column('C:C', 20)
    worksheet.set_column('D:E', 10)
    row = 0

    worksheet.write_row(row, 0, ['VARIABLE', 'C FILE', 'MODULE', 'OWNER', 'SIZE'], title_format)
    row += 1

    vari_dic_list = sorted(vari_dic_list, key=operator.itemgetter('Size'), reverse=True)
    for sym_dic in vari_dic_list:
        (lib_owner, format_entry) = find_lib_owner(sym_dic['Lib'])
        entry_format = benchbook.add_format(format_entry)
        worksheet.write_row(row , 0, [sym_dic['Sym'], sym_dic['File'],
                            sym_dic['Lib'], lib_owner, sym_dic['Size']], entry_format)
        row += 1

    worksheet.write_row(row, 0, ['TOTAL', '', '', ''], title_format)
    worksheet.write_formula(row, 4, '=SUM(E2:E' + str(row) + ')', title_format)

def get_mem_info(map_file):
    mem_map_text = ''
    sym_all_list = []
    global compiler_name
    global data_compress

    #1. get 'mem_map_text'
    with open(map_file, 'r') as f:
        mem_map_text = f.read()
        if not mem_map_text:
            print ('Can\'t parse map_file!')
            return
    map_flag = re.findall(r'Memory Map of the image', mem_map_text)
    if not map_flag:
        compiler_name = 'gcc'
        data_compress = 'no'
        get_sym_list_gcc(sym_all_list, map_file, mem_map_text)
    else:
        compiler_name = 'armcc'
        data_compress = 'yes'
        get_sym_list_armcc(sym_all_list, map_file, mem_map_text)

    #2. footprint.xlsx parse
    benchbook = xlsxwriter.Workbook('footprint.xlsx')
    # library (*.a) parse
    parse_library(sym_all_list, benchbook)
    # object file (*.o) parse
    parse_object(sym_all_list, benchbook)
    # symbol parse
    parse_symbol(sym_all_list, benchbook)
    benchbook.close()

def main():
    if len(sys.argv) > 1:
        map_file = sys.argv[1]
    else:
        file_list = os.listdir('.')
        for map_file in file_list:
            if r'.map' in map_file:
                break

    get_mem_info(map_file)

if __name__ == "__main__":
    main()
