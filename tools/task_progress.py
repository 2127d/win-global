"""Make task progress frames safe at arbitrarily small widths."""
import re
from xml.parsers import expat


def without_progress_endcaps(svg):
    """Remove fixed-width progress sides/corners, preserving other SVG artwork.

    KDE resizes the progress FrameSvg to the current percentage. Fixed-width
    left/right slices overlap when that width is smaller than their sum. A
    frame with only center/top/bottom slices can shrink all the way to zero.
    """
    data = svg.encode('utf-8')
    parser = expat.ParserCreate()
    stack, spans = [], []
    pattern = re.compile(r'(?:(?:north|south|east|west)-)?progress-(?:left|right|topleft|topright|bottomleft|bottomright)$')

    def start(name, attributes):
        offset = parser.CurrentByteIndex
        opening_end = data.index(b'>', offset) + 1
        stack.append((offset, opening_end, data[offset:opening_end].rstrip().endswith(b'/>'),
                      bool(pattern.fullmatch(attributes.get('id', '')))))

    def end(name):
        offset, opening_end, empty, remove = stack.pop()
        if remove:
            finish = opening_end if empty else data.index(b'>', parser.CurrentByteIndex) + 1
            line_start = data.rfind(b'\n', 0, offset) + 1
            line_end = data.find(b'\n', finish)
            if line_end >= 0 and not data[line_start:offset].strip() and not data[finish:line_end].strip():
                offset, finish = line_start, line_end + 1
            spans.append((offset, finish))

    parser.StartElementHandler = start
    parser.EndElementHandler = end
    parser.Parse(data, True)
    for start, end in sorted(spans, reverse=True):
        data = data[:start] + data[end:]
    return data.decode('utf-8')
