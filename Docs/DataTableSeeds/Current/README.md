# Current DataTable CSV Sources

This folder is the single editable CSV source for the current C++ row structs. Edit only these files, save as UTF-8 CSV, and reimport the matching Unreal DataTable one at a time.

The CSV files in the parent `DataTableSeeds` folder are retained only as historical backups. They include older headers and must not be used for future reimports.

## Asset mapping

| CSV | Unreal asset |
|---|---|
| `DT_Block.csv` | `/Game/Data/DT_Block` |
| `DT_BlockInteraction.csv` | `/Game/Data/DT_BlockInteraction` |
| `DT_Building.csv` | `/Game/Data/DT_Building` |
| `DT_DungeonAttribute.csv` | `/Game/Data/DT_DungeonAttribute` |
| `DT_Item.csv` | `/Game/Data/DT_Item` |
| `DT_Portal.csv` | `/Game/Data/DT_Portalfix` (unified portal and dungeon-layout definitions) |
| `DT_Research.csv` | `/Game/Data/DT_Research` |
| `DT_Resource.csv` | `/Game/Data/DT_Resource` |
| `DT_Scenario.csv` | `/Game/Data/DT_Scenario` |
| `DT_Skill.csv` | `/Game/Data/DT_Skill` |
| `DT_Work.csv` | `/Game/Data/DT_Work` |

`DT_Portalfix` is an intentional asset-name exception left from the earlier `DT_Portal` recovery. Always reimport `DT_Portal.csv` into `DT_Portalfix`, not into a new asset named `DT_Portal`. It now contains both portal spawn fields and dungeon-layout selection fields (`PresetAsset`, difficulty range, and `SelectionWeight`).

## Safe reimport procedure

1. Close every open DataTable editor and restart Unreal Editor after any C++ build. This prevents `HOTRELOAD` or `REINST` RowStruct references.
2. Open the target DataTable and confirm that the Row Editor does not say `None` and does not mention `HOTRELOAD`. Stop if either appears; fix the RowStruct before reimporting.
3. Edit the matching CSV in this folder. Keep the header row unchanged, preserve the `Name` column, and save it as UTF-8 CSV.
4. In the Content Browser, right-click the matching asset above and choose `Reimport`.
5. Verify the expected IDs and row count, then save the asset before moving to the next table.

Recommended order when rebuilding all test data:

`DT_Resource` -> `DT_Item` -> `DT_Block` -> `DT_BlockInteraction` -> `DT_Skill` -> `DT_Work` -> `DT_Building` -> `DT_Research` -> `DT_DungeonAttribute` -> `DT_Portalfix` -> `DT_Scenario`.

## Cross-reference rules

- `DT_BlockInteraction.ResultItemId` must exist in `DT_Item`.
- `DT_Work` cost resource IDs must exist in `DT_Resource`; reward item IDs must exist in `DT_Item`.
- `DT_Building.BuildWorkId` and `DT_Research.ResearchWorkId` must exist in `DT_Work`.
- `DT_DungeonAttribute.CoreShardItemId` must exist in `DT_Item`.
- `DT_Portal.PresetAsset` and `DT_Item` Blueprint class paths must refer to existing Unreal assets.
