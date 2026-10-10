"""Shared KDE defaults for restrained, readable desktop typography."""

FONT_FAMILY = 'Adwaita Sans'

def font(size=10, weight=400):
    return f'{FONT_FAMILY},{size},-1,5,{weight},0,0,0,0,0,0,0,0,0,0,1,,0,0'


# Leave fixed-width fonts alone so terminals and editors remain monospaced.
FONT_SETTINGS = {
    'General': {
        'font': font(),
        'menuFont': font(),
        'toolBarFont': font(),
        'smallestReadableFont': font(9),
        'taskbarFont': font(),
    },
    'WM': {'activeFont': font(10, 500)},
}
