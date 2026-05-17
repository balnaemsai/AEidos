**DataTable Seeds**

These CSV files are starter seed sheets for Unreal `DataTable` import/reimport.
You can edit them in Excel or Google Sheets, then export as `CSV UTF-8` and
reimport them into Unreal.

Files:
- `DT_Resource_seed.csv`
- `DT_Skill_seed.csv`
- `DT_Work_seed.csv`
- `DT_Building_seed.csv`
- `DT_Portal_seed.csv`

General rules:
- The first column `Name` is the Unreal DataTable row name.
- It is recommended that row names match ID fields such as `ResourceId`,
  `SkillId`, `WorkId`, `BuildingId`, and `PortalId`.
- Export back to `CSV UTF-8` before Unreal import/reimport.

Notes:
- `DT_Work_seed.csv` and `DT_Building_seed.csv` include array/struct-style
  fields written in Unreal CSV import format.
- `BuildingActorClass`, `ConstructionSiteActorClass`, and `PortalActorClass`
  currently point to test blueprint classes already present in the project.
- Replace test blueprint paths later when real gameplay blueprints are ready.

Starter content:
- Resources: `EP`, `Food`, `Wood`, `Stone`, `Shard`
- Skills: `Construction`, `Gathering`, `Endurance`, `Slash`, `Firebolt`
- Work rows: 5 starter rows
- Buildings: `Hut`, `Stockpile`, `Workbench`, `TrainingDummy`, `WatchPost`
- Portals: 5 starter rows

Recommended import order:
1. `DT_Resource`
2. `DT_Skill`
3. `DT_Work`
4. `DT_Building`
5. `DT_Portal`

Verification:
- Import test assets are created under `/Game/Data/ImportTests`.
- Verification script:
  `C:\Users\BAL\Documents\Unreal Projects\AEidos\Scripts\verify_datatable_seed_import.py`
