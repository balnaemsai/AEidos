import unreal
out_lines = []
for path in ['/Game/Data/DT_Portal.DT_Portal','/Game/Data/ImportTests/DT_Portal_ImportTest.DT_Portal_ImportTest']:
    asset = unreal.load_asset(path)
    if not asset:
        out_lines.append(f'{path}|LOAD_FAILED')
        continue
    row_struct = asset.get_editor_property('row_struct')
    row_name = row_struct.get_name() if row_struct else 'None'
    row_path = row_struct.get_path_name() if row_struct else 'None'
    out_lines.append(f'{path}|{row_name}|{row_path}')
with open(r'C:\Users\BAL\Documents\Unreal Projects\AEidos\Scripts\inspect_portal_dt_out.txt', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out_lines))
