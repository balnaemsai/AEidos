import unreal
out_path = r'C:\Users\BAL\Documents\Unreal Projects\AEidos\Saved\inspect_editor_shortcuts.txt'
lines = []
cls = getattr(unreal, 'EditorKeyboardShortcutSettings', None)
lines.append('HAS_CLASS=' + str(bool(cls)))
if cls:
    obj = unreal.get_default_object(cls)
    props = [p for p in dir(obj) if not p.startswith('_')]
    lines.append('PROP_COUNT=' + str(len(props)))
    lines.extend(props)
with open(out_path, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))
