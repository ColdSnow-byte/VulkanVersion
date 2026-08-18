import struct

names = ['vert_10', 'frag_10']

for name in names:
    with open(name + '.spv', 'rb') as f:
        data = f.read()

    while len(data) % 4 != 0:
        data += b'\x00'

    words = []
    for i in range(0, len(data), 4):
        (v,) = struct.unpack('<I', data[i:i+4])
        words.append(v)

    lines = []
    lines.append('// Auto-generated SPIR-V shader binary')
    lines.append('#pragma once')
    lines.append('#include <cstdint>')
    lines.append('')
    lines.append('static const uint32_t %s_spv[] = {' % name)
    for i, w in enumerate(words):
        comma = ',' if i < len(words) - 1 else ''
        lines.append('    0x%08X%s' % (w, comma))
    lines.append('};')

    with open(name + '.h', 'w') as f:
        f.write('\n'.join(lines) + '\n')

    print('Generated %s.h (%d words)' % (name, len(words)))
