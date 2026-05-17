import unreal


PROJECT_ROOT = r"C:\Users\BAL\Documents\Unreal Projects\AEidos"
SEED_DIR = PROJECT_ROOT + r"\Docs\DataTableSeeds"
DEST_PATH = "/Game/Data/ImportTests"


def log(msg):
    unreal.log("[DataTableSeedVerify] " + str(msg))


def fail(msg):
    unreal.log_error("[DataTableSeedVerify] " + str(msg))
    raise RuntimeError(msg)


def ensure_folder(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_row_struct(struct_name: str):
    base_name = struct_name[1:] if struct_name.startswith("F") else struct_name
    candidates = [
        f"/Script/AEidos.{base_name}",
        f"/Script/AEidos.{struct_name}",
        f"/Script/AEidos.{base_name}Row",
        f"/Script/AEidos.{struct_name}Row",
    ]
    for candidate in candidates:
        struct_obj = unreal.load_object(None, candidate)
        if struct_obj:
            return struct_obj
    fail(f"Could not load row struct for {struct_name}; tried {candidates}")


def import_csv(csv_filename: str, asset_name: str, struct_name: str):
    csv_path = SEED_DIR + "\\" + csv_filename
    row_struct = load_row_struct(struct_name)

    factory = unreal.CSVImportFactory()
    settings = factory.automated_import_settings

    enum_type = None
    for enum_name in ("ECSVImportType", "CSVImportType"):
        enum_type = getattr(unreal, enum_name, None)
        if enum_type:
            log(f"Using import enum {enum_name}")
            break

    if enum_type:
        for enum_value_name in ("ECSV_DATA_TABLE", "DATA_TABLE"):
            enum_value = getattr(enum_type, enum_value_name, None)
            if enum_value is not None:
                settings.import_type = enum_value
                break
        else:
            enum_values = [name for name in dir(enum_type) if name.isupper()]
            fail(f"Could not find data table enum value on {enum_type}; available={enum_values}")
    else:
        if hasattr(settings, "import_type"):
            settings.import_type = 0
            log("Falling back to numeric import_type=0 for DataTable import")
        else:
            fail(f"automated_import_settings has no import_type field; fields={dir(settings)}")

    settings.import_row_struct = row_struct
    if hasattr(settings, "b_force_automated_import"):
        settings.b_force_automated_import = True
    else:
        log("CSVImportSettings has no b_force_automated_import; continuing without it")
    factory.automated_import_settings = settings

    task = unreal.AssetImportTask()
    task.filename = csv_path
    task.destination_path = DEST_PATH
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.factory = factory

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if task.imported_object_paths:
        log(f"Imported {csv_filename} -> {task.imported_object_paths}")
        return

    fail(f"Import failed for {csv_filename}")


def main():
    ensure_folder(DEST_PATH)

    imports = [
        ("DT_Resource_seed.csv", "DT_Resource_ImportTest", "FResourceDefinitionRow"),
        ("DT_Skill_seed.csv", "DT_Skill_ImportTest", "FSkillDefinitionRow"),
        ("DT_Work_seed.csv", "DT_Work_ImportTest", "FWorkDefinitionRow"),
        ("DT_Building_seed.csv", "DT_Building_ImportTest", "FBuildingDefinitionRow"),
        ("DT_Portal_seed.csv", "DT_Portal_ImportTest", "FPortalDefinitionRow"),
    ]

    for csv_filename, asset_name, struct_name in imports:
        import_csv(csv_filename, asset_name, struct_name)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("All seed imports completed successfully")


if __name__ == "__main__":
    main()
