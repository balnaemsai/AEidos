import unreal


PROJECT_ROOT = r"C:\Users\BAL\Documents\Unreal Projects\AEidos"
SEED_ROOT = PROJECT_ROOT + r"\Docs\DataTableSeeds\Current"
IMPORTS = (
    ("DT_Skill.csv", "DT_Skill", "SkillDefinitionRow"),
    ("DT_Item.csv", "DT_Item", "ItemDefinitionRow"),
    ("DT_Work.csv", "DT_Work", "WorkDefinitionRow"),
    ("DT_Building.csv", "DT_Building", "BuildingDefinitionRow"),
    ("DT_Research.csv", "DT_Research", "ResearchDefinitionRow"),
    ("DT_Portal.csv", "DT_Portalfix", "PortalDefinitionRow"),
)


def get_struct(struct_name):
    for object_path in (f"/Script/AEidos.{struct_name}", f"/Script/AEidos.F{struct_name}"):
        value = unreal.load_object(None, object_path)
        if value:
            return value
    raise RuntimeError(f"Could not load row struct for {struct_name}")


def import_table(csv_name, asset_name, struct_name):
    factory = unreal.CSVImportFactory()
    settings = factory.automated_import_settings
    enum_type = getattr(unreal, "ECSVImportType", None) or getattr(unreal, "CSVImportType", None)
    settings.import_type = getattr(enum_type, "ECSV_DATA_TABLE", getattr(enum_type, "DATA_TABLE", 0)) if enum_type else 0
    settings.import_row_struct = get_struct(struct_name)
    factory.automated_import_settings = settings

    task = unreal.AssetImportTask()
    task.filename = SEED_ROOT + "\\" + csv_name
    task.destination_path = "/Game/Data"
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.factory = factory
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Import failed: {csv_name}")
    unreal.log("[CurrentDataTableReimport] {} -> {}".format(csv_name, task.imported_object_paths))


def main():
    for entry in IMPORTS:
        import_table(*entry)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


if __name__ == "__main__":
    main()
