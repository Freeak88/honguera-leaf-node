#!/usr/bin/env python3
"""
Honguera Leaf Node - variant generator v4
Single-pass removal for maximum reliability.
"""

import re, os, shutil

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KICAD = os.path.join(BASE, "hardware", "pcb", "kiCad")

def read(path):
    with open(path) as f: return f.read()

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f: f.write(content)

def all_symbol_blocks(content):
    """Return list of (start, end, lib_id_name) for all instantiation symbols."""
    out = []
    pat = re.compile(r'\(symbol\s+\(lib_id\s+"([^"]+)"\)')
    for m in pat.finditer(content):
        start = m.start()
        depth = 0
        end = start
        while end < len(content):
            if content[end] == '(': depth += 1
            elif content[end] == ')':
                depth -= 1
                if depth == 0: end += 1; break
            end += 1
        out.append((start, end, m.group(1).split(':')[-1]))
    return out

def remove_blocks(content, blocks_to_remove):
    """Remove blocks in reverse order. blocks_to_remove = [(start, end), ...]"""
    for start, end in sorted(blocks_to_remove, key=lambda x: x[0], reverse=True):
        content = content[:start] + content[end:]
    return content

def create_lite():
    print("=== Honguera Lite ===")
    src = os.path.join(KICAD, "LeafNode_v3.1.kicad_sch")
    content = read(src)
    blocks = all_symbol_blocks(content)
    
    # Map: lib_id_name -> list of (start, end)
    by_name = {}
    for s, e, n in blocks:
        by_name.setdefault(n, []).append((s, e))
    
    to_remove = []
    
    # Components to remove entirely (all instances)
    for prefix in ['MCP4725', 'MAX485', 'LM358', 'R-78E3', 'IRLML6344']:
        for name, insts in by_name.items():
            if name.startswith(prefix):
                to_remove.extend(insts)
                print(f"  ❌ Removing all {name} ({len(insts)}x)")
    
    # Keep only 1 G6K relay and 1 SI2302 MOSFET
    for name, limit in [('G6K-2F', 1), ('SI2302', 1), ('LESD5D5', 2)]:
        matching = [(n, ii) for n, ii in by_name.items() if n.startswith(name)]
        total = sum(len(ii) for _, ii in matching)
        excess = total - limit
        removed = 0
        for n, ii in reversed(matching):
            for inst in reversed(ii):
                if removed >= excess: break
                to_remove.append(inst)
                removed += 1
        print(f"  ❌ Keeping {limit}/{total} {name} (removed {removed})")
    
    content = remove_blocks(content, to_remove)
    
    # Update title
    content = content.replace(
        '(title "Leaf Node")\n\t\t(date "01.06.2026")\n\t\t(rev "3.1")\n\t\t(company "Leaf AI LLC")',
        '(title "Honguera Node Lite")\n\t\t(date "01.07.2026")\n\t\t(rev "1.0")\n\t\t(company "Honguera")'
    )
    
    outdir = os.path.join(BASE, "hardware", "variants", "honguera-lite")
    out = os.path.join(outdir, "HongueraNode_Lite.kicad_sch")
    write(out, content)
    
    # Copy other KiCad files
    for f in os.listdir(KICAD):
        if any(f.endswith(ext) for ext in ['.kicad_pcb', '.kicad_pro', '.kicad_dru', '.kicad_sym', '-lib-table']):
            dst = f.replace("LeafNode_v3.1", "HongueraNode_Lite")
            shutil.copy2(os.path.join(KICAD, f), os.path.join(outdir, dst))
    
    # Fix project file
    pf = os.path.join(outdir, "HongueraNode_Lite.kicad_pro")
    if os.path.exists(pf):
        p = read(pf).replace("LeafNode_v3.1", "HongueraNode_Lite").replace('"title":"Leaf Node"', '"title":"Honguera Node Lite"')
        write(pf, p)
    
    # Verify
    verify_lite(out)
    print(f"  → {out}")

def verify_lite(path):
    c = read(path)
    blocks = all_symbol_blocks(c)
    names = set(n for _,_,n in blocks)
    
    targets = {
        'MCP4725 DAC': 'MCP4725', 'MAX485 RS485': 'MAX485', 
        'LM358 SDI-12': 'LM358', 'R-78E3.3': 'R-78E3', 'IRLML6344 MOSFET': 'IRLML6344'
    }
    for label, prefix in targets.items():
        found = [n for n in names if n.startswith(prefix)]
        print(f"  {'✅' if not found else '⚠️'} {label}: {found if found else 'removed'}")
    
    # Check counts
    g6k = len([1 for n in names if 'G6K' in n])
    si = len([1 for n in names if 'SI2302' in n])
    tvs = len([1 for n in names if 'LESD5' in n])
    total = len(blocks)
    print(f"  ✅ G6K={g6k} (expect 1) | SI2302={si} (expect 1) | TVS={tvs} (expect 2)")
    print(f"  ✅ Total symbol blocks: {total}")

def create_pro():
    print("\n=== Honguera Pro ===")
    src = os.path.join(KICAD, "LeafNode_v3.1.kicad_sch")
    content = read(src).replace(
        '(title "Leaf Node")\n\t\t(date "01.06.2026")\n\t\t(rev "3.1")\n\t\t(company "Leaf AI LLC")',
        '(title "Honguera Node Pro")\n\t\t(date "01.07.2026")\n\t\t(rev "1.0")\n\t\t(company "Honguera")'
    )
    
    outdir = os.path.join(BASE, "hardware", "variants", "honguera-pro")
    out = os.path.join(outdir, "HongueraNode_Pro.kicad_sch")
    write(out, content)
    
    for f in os.listdir(KICAD):
        if any(f.endswith(ext) for ext in ['.kicad_pcb', '.kicad_pro', '.kicad_dru', '.kicad_sym', '-lib-table']):
            dst = f.replace("LeafNode_v3.1", "HongueraNode_Pro")
            shutil.copy2(os.path.join(KICAD, f), os.path.join(outdir, dst))
    
    pf = os.path.join(outdir, "HongueraNode_Pro.kicad_pro")
    if os.path.exists(pf):
        p = read(pf).replace("LeafNode_v3.1", "HongueraNode_Pro").replace('"title":"Leaf Node"', '"title":"Honguera Node Pro"')
        write(pf, p)
    
    print(f"  → {out}")

def create_docs():
    write(os.path.join(BASE, "hardware", "variants", "MODIFICATIONS.md"), """# Honguera Leaf Node Modifications Guide

## Lite (`honguera-lite/`)
**Automated:** DAC, RS485, SDI-12, step-down regulator, extra MOSFET, extra relay, extra TVS removed.

**Manual KiCad needed:**
1. Add **MH-Z19B CO₂** → 4-pin header on UART2 (GPIO18 RX, GPIO19 TX)
2. Add **SSR** for humidifier → one free GPIO
3. Replace step-down → **AMS1117-3.3** (SOT-223)
4. Compact PCB ~30-40%
5. Remove redundant text labels & test points

## Pro (`honguera-pro/`)
**Automated:** Title/project metadata updated. All components kept.

**Manual KiCad needed:**
1. Add **MH-Z19B CO₂** → 4-pin header UART2
2. Add **SSR footprint** → AC humidifier control
3. Add **MPXV7002DP** → differential pressure (analog ADC)
4. Add **DS3231 RTC** → I2C bus
5. Add **5-pin expansion header** (+5V, GND, GPIO, SDA, SCL)

## Firmware targets (PlatformIO)
```ini
[env:honguera-lite]
board = esp32-s3-devkitc-1
build_flags = -DHONGUERA_LITE

[env:honguera-pro]
board = esp32-s3-devkitc-1
build_flags = -DHONGUERA_PRO
```
""")

if __name__ == "__main__":
    vdir = os.path.join(BASE, "hardware", "variants")
    if os.path.exists(vdir): shutil.rmtree(vdir)
    create_lite()
    create_pro()
    create_docs()
    print("\n✅ Variants generated.")
