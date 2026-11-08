#!/usr/bin/env python3
"""Render this V6's C keymap as PNGs. Reads source, not the keyboard's VIA EEPROM."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import time

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
DEFAULT = ROOT / 'keyboards/keychron/v6/ansi_encoder/keymaps/bregmi/keymap.c'
TRANSPARENT = {'_______', 'KC_TRNS', 'KC_TRANSPARENT'}
DISABLED = {'XXXXXXX', 'KC_NO'}
LABELS = {
    'KC_ESC': 'Esc', 'KC_BRID': 'Screen\nlight −', 'KC_BRIU': 'Screen\nlight +',
    'KC_MPRV': 'Prev.\ntrack', 'KC_MPLY': 'Play /\npause', 'KC_MNXT': 'Next\ntrack',
    'KC_MCTL': 'Mission\nControl', 'KC_LPAD': 'Launchpad',
    'KC_MUTE': 'Mute', 'KC_VOLD': 'Volume −', 'KC_VOLU': 'Volume +',
    'KC_PSCR': 'Print\nScreen', 'TURBO': 'Auto\nclicker', 'MOUSE_TURBO_CLICK': 'Auto\nclicker',
    'AC_TOGG': 'Auto\ncorrect\ntoggle', 'KC_SLEP': 'Sleep',
    'KC_GRV': '` ~', 'KC_MINS': '− _', 'KC_EQL': '= +', 'KC_BSPC': 'Backspace',
    'KC_INS': 'Insert', 'KC_HOME': 'Home', 'KC_PGUP': 'Page up', 'KC_NUM': 'Num lock',
    'KC_PSLS': 'Num /', 'KC_PAST': 'Num *', 'KC_PMNS': 'Num −', 'KC_TAB': 'Tab',
    'KC_LBRC': '[ {', 'KC_RBRC': '] }', 'KC_BSLS': '\\ |', 'KC_DEL': 'Delete',
    'KC_END': 'End', 'KC_PGDN': 'Page down', 'KC_CAPS': 'Caps lock', 'KC_SCLN': '; :',
    'KC_QUOT': "' \"", 'KC_ENT': 'Enter', 'KC_PPLS': 'Num +',
    'KC_LSFT': 'Left Shift', 'KC_RSFT': 'Right Shift', 'KC_COMM': ', <', 'KC_DOT': '. >',
    'KC_SLSH': '/ ?', 'KC_UP': '↑', 'KC_LEFT': '←', 'KC_DOWN': '↓', 'KC_RGHT': '→',
    'KC_LCTL': 'Left Ctrl', 'KC_RCTL': 'Right Ctrl', 'KC_SPC': 'Space',
    'KC_PDOT': 'Num .', 'KC_PENT': 'Num Enter', 'NK_TOGG': 'N-key rollover\ntoggle',
    'RM_TOGG': 'RGB\non / off', 'RM_NEXT': 'RGB effect\nnext', 'RM_PREV': 'RGB effect\nprev.',
    'RM_VALD': 'RGB\nlevel −', 'RM_VALU': 'RGB\nlevel +',
    'RM_HUED': 'Hue −', 'RM_HUEU': 'Hue +', 'RM_SATD': 'Color sat.\n−',
    'RM_SATU': 'Color sat.\n+', 'RM_SPDD': 'Effect\nspeed −', 'RM_SPDU': 'Effect\nspeed +',
}


def balanced(text, start):
    """Read nested (...) or {...}, preserving nested macro arguments."""
    closer = {'(': ')', '{': '}'}[text[start]]
    opener = text[start]
    depth = 1
    for i in range(start + 1, len(text)):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if not depth:
                return text[start + 1:i]
    raise ValueError('Unbalanced C initializer')


def split_args(text):
    result, start, depth = [], 0, 0
    for i, char in enumerate(text):
        if char in '({[':
            depth += 1
        elif char in ')}]':
            depth -= 1
        elif char == ',' and not depth:
            result.append(text[start:i].strip())
            start = i + 1
    if text[start:].strip():
        result.append(text[start:].strip())
    return result


def parse_source(path):
    text = path.read_text()
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    aliases = dict(re.findall(r'^\s*#define\s+(\w+)[ \t]+([^\n]+)', text, re.M))
    match = re.search(r'\bkeymaps\s*\[[^;=]+?=\s*\{', text)
    if not match:
        raise ValueError('No keymaps array found')
    body = balanced(text, match.end() - 1)
    layers = {}
    for match in re.finditer(r'\[(\w+)\]\s*=\s*(LAYOUT\w*)\s*\(', body):
        name, layout = match.groups()
        if name in layers:
            raise ValueError(f'Duplicate layer {name}; conditional C is not supported')
        layers[name] = (layout, split_args(balanced(body, match.end() - 1)))
    if not layers:
        raise ValueError('No named LAYOUT initializers found')
    encoders = {}
    match = re.search(r'\bencoder_map\s*\[[^;=]+?=\s*\{', text)
    if match:
        body = balanced(text, match.end() - 1)
        for match in re.finditer(r'\[(\w+)\]\s*=\s*\{', body):
            name = match[1]
            entry = balanced(body, match.end() - 1)
            pairs = []
            for pair in re.finditer(r'ENCODER_CCW_CW\s*\(', entry):
                args = split_args(balanced(entry, pair.end() - 1))
                if len(args) != 2:
                    raise ValueError('Encoder mapping must have two directions')
                pairs.append(args)
            encoders[name] = pairs
    return layers, encoders, aliases


def label(code, layer, aliases, seen=()):
    mac = layer.startswith('MAC')
    if code in TRANSPARENT:
        return 'Transparent'
    if code in DISABLED:
        return 'No action'
    if code in LABELS:
        return LABELS[code]
    if code in aliases and code not in seen:
        return label(aliases[code].strip(), layer, aliases, (*seen, code))
    if code in {'KC_LCMD', 'KC_RCMD', 'KC_LWIN', 'KC_RWIN', 'KC_LGUI', 'KC_RGUI'}:
        return ('Left ' if code.startswith('KC_L') else 'Right ') + ('Cmd' if mac else 'Win')
    if code in {'KC_LOPT', 'KC_ROPT', 'KC_LALT', 'KC_RALT'}:
        return ('Left ' if code.startswith('KC_L') else 'Right ') + ('Option' if mac else 'Alt')
    match = re.fullmatch(r'MO\((\w+)\)', code)
    if match:
        return 'Fn (hold)\n' + match[1].replace('_', ' ')
    match = re.fullmatch(r'(LGUI|LCMD|LCTL|LALT|LSFT|S|C|A|G)\((.*)\)', code)
    if match:
        mods = {'LGUI': 'Cmd' if mac else 'Win', 'LCMD': 'Cmd', 'LCTL': 'Ctrl',
                'LALT': 'Option' if mac else 'Alt', 'LSFT': 'Shift', 'S': 'Shift',
                'C': 'Ctrl', 'A': 'Alt', 'G': 'Cmd' if mac else 'Win'}
        return mods[match[1]] + ' + ' + label(match[2], layer, aliases, seen)
    if re.fullmatch(r'KC_P[0-9]', code):
        return 'Num ' + code[-1]
    if re.fullmatch(r'KC_([A-Z0-9]|F\d+)', code):
        return code[3:]
    return code  # Unknown/custom macros stay visible rather than being guessed.


def font(size):
    candidates = ['/System/Library/Fonts/Supplemental/Arial.ttf',
                  '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 'DejaVuSans.ttf']
    for name in candidates:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default(size=size)


def centered(draw, box, text, color, size=17):
    x0, y0, x1, y1 = box
    # Wrap long raw keycodes/labels and fit within every physical key.
    for current in range(size, 8, -1):
        f = font(current)
        lines = []
        for line in text.split('\n'):
            while draw.textlength(line, font=f) > x1 - x0 - 10:
                cut = len(line) - 1
                while cut > 1 and draw.textlength(line[:cut], font=f) > x1 - x0 - 10:
                    cut -= 1
                space = line.rfind(' ', 0, cut + 1)
                if space > 0:
                    cut = space
                lines.append(line[:cut]); line = line[cut:].lstrip()
            lines.append(line)
        value = '\n'.join(lines)
        bounds = draw.multiline_textbbox((0, 0), value, font=f, spacing=3)
        if bounds[3] - bounds[1] <= y1 - y0 - 12:
            break
    draw.multiline_text(((x0+x1-bounds[2])/2, (y0+y1-(bounds[3]-bounds[1]))/2-bounds[1]),
                        value, fill=color, font=f, align='center', spacing=3)


def generate(keymap, keyboard, output):
    layers, encoders, aliases = parse_source(keymap)
    layouts = json.loads(keyboard.read_text())['layouts']
    for name, (layout, codes) in layers.items():
        if layout not in layouts or len(codes) != len(layouts[layout]['layout']):
            raise ValueError(f'{name}: key count/layout does not match keyboard.json')
    images, manifest = [], {}
    output.mkdir(parents=True, exist_ok=True)
    for name, (layout, codes) in layers.items():
        positions = layouts[layout]['layout']
        base = name.removesuffix('_FN') + '_BASE' if name.endswith('_FN') else name
        if any(c in TRANSPARENT for c in codes) and base not in layers:
            raise ValueError(f'Cannot resolve transparent keys for {name}; expected {base}')
        unit, margin, top = 80, 32, 108
        width = round(max(p['x']+p.get('w',1) for p in positions)*unit)+2*margin
        height = round(max(p['y']+p.get('h',1) for p in positions)*unit)+top+95
        img = Image.new('RGB', (width,height), '#101720'); draw = ImageDraw.Draw(img)
        title = name.replace('MAC', 'macOS').replace('WIN', 'Windows').replace('_', ' · ')
        draw.text((margin,18), title, font=font(29), fill='#f2f6fc')
        draw.text((margin,57), 'Source keymap • solid = assigned here • muted = inherited • unassigned = no action', font=font(16), fill='#a9bacd')
        resolved = []
        for i, (code, pos) in enumerate(zip(codes, positions)):
            inherited = code in TRANSPARENT
            effective = layers[base][1][i] if inherited and base != name else code
            description = label(effective, name, aliases)
            fill, border, ink = '#273647', '#526477', '#eef4fa'
            if effective in DISABLED:
                fill, border, ink = '#1b222b', '#34404c', '#82909e'
            elif inherited:
                fill, border, ink = '#19232e', '#344353', '#94a8bd'
            elif effective in {'TURBO','MOUSE_TURBO_CLICK','AC_TOGG'}:
                fill, border = '#174d4c', '#58cfc0'
            elif effective.startswith('RM_'):
                fill, border = '#34314e', '#8c82bf'
            x,y = margin+pos['x']*unit, top+pos['y']*unit
            box=(x+3,y+3,x+pos.get('w',1)*unit-3,y+pos.get('h',1)*unit-3)
            draw.rounded_rectangle(box,radius=8,fill=fill,outline=border,width=2)
            centered(draw,box,description,ink)
            resolved.append({'position':i, 'code':code, 'effective':effective, 'label':description, 'inherited':inherited})
        knob = []
        for i, pair in enumerate(encoders.get(name, [])):
            pair = [encoders.get(base, [pair])[i][j] if c in TRANSPARENT else c for j,c in enumerate(pair)]
            knob.append(f'Knob {i+1}: counterclockwise = {label(pair[0],name,aliases).replace(chr(10)," ")}   |   clockwise = {label(pair[1],name,aliases).replace(chr(10)," ")}')
        draw.text((margin,height-75),'  /  '.join(knob),font=font(19),fill='#e1ebf6')
        draw.text((margin,height-41),'Knob press is the isolated key left of Print Screen. Source defaults only; saved VIA remaps are not read.',font=font(15),fill='#a9bacd')
        img.save(output/f'{name.lower()}.png'); images.append(img)
        manifest[name] = {'keys':resolved,'encoder':encoders.get(name,[])}
    overview = Image.new('RGB',(max(im.width for im in images),sum(im.height for im in images)+20*(len(images)-1)),'#101720')
    y=0
    for im in images:
        overview.paste(im,(0,y)); y+=im.height+20
    overview.save(output/'all_layers.png')
    (output/'layers.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Rendered {len(layers)} layers → {output}',flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--keymap',type=Path,default=DEFAULT)
    parser.add_argument('--keyboard',type=Path,default=ROOT/'keyboards/keychron/v6/ansi_encoder/keyboard.json')
    parser.add_argument('--output',type=Path,default=Path(__file__).resolve().parent/'output')
    parser.add_argument('--watch',action='store_true',help='Regenerate when source keymap or keyboard geometry changes')
    args=parser.parse_args()
    previous=None
    while True:
        try:
            digest=hashlib.sha256(args.keymap.read_bytes()+args.keyboard.read_bytes()).digest()
            if digest != previous:
                generate(args.keymap,args.keyboard,args.output)
                previous=digest
        except (ValueError,KeyError,OSError) as error:
            if not args.watch:
                parser.exit(1,f'Error: {error}\n')
            print(f'Waiting for valid source: {error}',flush=True)
        if not args.watch:
            break
        time.sleep(1)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
