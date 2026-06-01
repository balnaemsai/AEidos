import unreal


PROJECT_ROOT = r"C:\Users\BAL\Documents\Unreal Projects\AEidos"
CSV_PATH = PROJECT_ROOT + r"\Docs\DataTableSeeds\DT_Portal_seed.csv"
DEST_PATH = "/Game/Data"
DEST_NAME = "DT_Portal"


def log(msg):
    unreal.log("[PortalReimport] " + str(msg))


def fail(msg):
    unreal.log_error("[PortalReimport] " + str(msg))
    raise RuntimeError(msg)


def load_row_struct():
    candidates = [
        "/Script/AEidos.PortalDefinitionRow",
        "/Script/AEidos.FPortalDefinitionRow",
    ]
    for candidate in candidates:
        struct_obj = unreal.load_object(None, candidate)
        if struct_obj:
            return struct_obj
    fail(f"Could not load PortalDefinitionRow struct; tried {candidates}")


def main():
    row_struct = load_row_struct()

    factory = unreal.CSVImportFactory()
    settings = factory.automated_import_settings

    enum_type = None
    for enum_name in ("ECSVImportType", "CSVImportType"):
        enum_type = getattr(unreal, enum_name, None)
        if enum_type:
            break

    if enum_type:
        for enum_value_name in ("ECSV_DATA_TABLE", "DATA_TABLE"):
            enum_value = getattr(enum_type, enum_value_name, None)
            if enum_value is not None:
                settings.import_type = enum_value
                break
        else:
            settings.import_type = 0
    else:
        settings.import_type = 0

    settings.import_row_struct = row_struct
    factory.automated_import_settings = settings

    task = unreal.AssetImportTask()
    task.filename = CSV_PATH
    task.destination_path = DEST_PATH
    task.destination_name = DEST_NAME
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.factory = factory

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not task.imported_object_paths:
        fail("DT_Portal import failed")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Imported DT_Portal from {CSV_PATH} -> {task.imported_object_paths}")


if __name__ == "__main__":
    main()
