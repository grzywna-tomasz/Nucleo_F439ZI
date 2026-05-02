from elftools.elf.elffile import ELFFile
import json

def get_section_ranges(elf):
    ranges = {}
    for sec in elf.iter_sections():
        if sec.name in ('.data', '.bss'):
            start = sec['sh_addr']
            end = start + sec['sh_size']
            ranges[sec.name] = (start, end)
    return ranges

def addr_in_ram(addr, ranges):
    for name, (start, end) in ranges.items():
        if start <= addr < end:
            return name
    return None

def resolve_type(die, dwarfinfo):
    """Recursively resolve type names (safe version)"""
    if die is None:
        return "unknown"

    name = die.attributes.get('DW_AT_name')
    name = name.value.decode() if name else ""

    def get_target(d):
        if 'DW_AT_type' in d.attributes:
            return d.get_DIE_from_attribute('DW_AT_type')
        return None

    if die.tag == 'DW_TAG_base_type':
        return name or "base"

    elif die.tag == 'DW_TAG_pointer_type':
        target = get_target(die)
        if target:
            return resolve_type(target, dwarfinfo) + "*"
        else:
            return "void*"

    elif die.tag == 'DW_TAG_const_type':
        target = get_target(die)
        if target:
            return "const " + resolve_type(target, dwarfinfo)
        else:
            return "const"

    elif die.tag == 'DW_TAG_array_type':
        target = get_target(die)
        base = resolve_type(target, dwarfinfo) if target else "unknown"
        return base + "[]"

    elif die.tag in ('DW_TAG_structure_type', 'DW_TAG_union_type'):
        return name or "struct"

    elif die.tag == 'DW_TAG_typedef':
        if name:
            return name
        target = get_target(die)
        return resolve_type(target, dwarfinfo) if target else "typedef"

    else:
        target = get_target(die)
        if target:
            return resolve_type(target, dwarfinfo)

    return name or "unknown"

def save_to_json(data):
    with open("variables.json", "w") as writer:
        json.dump(data, writer, indent=4)

def main(filename):
    with open(filename, 'rb') as f:
        elf = ELFFile(f)

        symtab = elf.get_section_by_name('.symtab')
        dwarfinfo = elf.get_dwarf_info()
        ram_ranges = get_section_ranges(elf)

        # Build address -> DIE map
        addr_to_die = {}

        for CU in dwarfinfo.iter_CUs():
            for DIE in CU.iter_DIEs():
                if DIE.tag == 'DW_TAG_variable':
                    loc = DIE.attributes.get('DW_AT_location')
                    if not loc:
                        continue

                    # Simple handling: direct address (common in embedded)
                    if loc.form == 'DW_FORM_exprloc':
                        expr = loc.value
                        if expr and expr[0] == 0x03:  # DW_OP_addr
                            addr = int.from_bytes(expr[1:], 'little')
                            addr_to_die[addr] = DIE

        out_dict = {}
        for sym in symtab.iter_symbols():
            if sym['st_info']['type'] != 'STT_OBJECT':
                continue


            addr = sym['st_value']
            size = sym['st_size']
            name = sym.name

            section = addr_in_ram(addr, ram_ranges)
            if not section:
                continue

            die = addr_to_die.get(addr)
            if die and 'DW_AT_type' in die.attributes:
                type_die = die.get_DIE_from_attribute('DW_AT_type')
                type_name = resolve_type(type_die, dwarfinfo)
            else:
                type_name = "unknown"
            
            out_dict.update({name: {"addr": addr, "size": size, "type": type_name}})
        
        save_to_json(out_dict)

if __name__ == "__main__":
    import sys
    main("../../build/Debug/nucleo.elf")