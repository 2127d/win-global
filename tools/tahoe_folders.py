"""Full-color folder variants derived from Tahoe folder artwork (GPL-3.0)."""


def apply(theme):
    directory = theme / 'places/scalable'
    base = (directory / 'folder.svg').read_text()
    # Keep the existing folder silhouette, shading and paper edge. The archive
    # zipper and bookmark ribbon stay legible at the chooser's 32px size.
    emblems = {
        'folder-tar': '''
 <g id="archive-zipper">
  <rect x="7.45" y="4.76" width="2.03" height="6.88" rx=".32" fill="#084ba4" opacity=".55"/>
  <path d="M7.63 5.02h.84v.85h-.84zm.84.85h.84v.85h-.84zm-.84.85h.84v.85h-.84zm.84.85h.84v.85h-.84zm-.84.85h.84v.85h-.84z" fill="#e7f3ff"/>
  <rect x="7.27" y="9.18" width="2.39" height="3.18" rx=".65" fill="#064093" opacity=".3"/>
  <rect x="7.27" y="8.98" width="2.39" height="3.18" rx=".65" fill="#d8e9fb"/>
  <rect x="7.89" y="10.3" width="1.15" height="1.15" rx=".25" fill="#256bc0"/>
 </g>
''',
        'folder-bookmark': '''
 <g id="bookmark-ribbon">
  <path d="M6.62 5.95h3.69v6.98a.24.24 0 0 1-.39.19l-1.45-1.13-1.46 1.13a.24.24 0 0 1-.39-.19z" fill="#064093" opacity=".3"/>
  <path d="M6.62 5.7h3.69v6.98a.24.24 0 0 1-.39.19l-1.45-1.13-1.46 1.13a.24.24 0 0 1-.39-.19z" fill="#ffd166"/>
  <path d="M6.62 5.7h3.69v.4H6.62z" fill="#fff2cc"/>
 </g>
''',
    }
    for name, emblem in emblems.items():
        (directory / (name + '.svg')).write_text(base.replace('</svg>', emblem + '</svg>'))
