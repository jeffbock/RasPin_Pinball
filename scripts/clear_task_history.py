import sqlite3

db = r'C:\Users\jeffd\AppData\Roaming\Code\User\workspaceStorage\d7353779e8a79f97c29cc5af19ac4515\state.vscdb'
con = sqlite3.connect(db)
cur = con.cursor()

cur.execute("SELECT key FROM ItemTable WHERE key LIKE '%task%' OR key LIKE '%Task%'")
rows = cur.fetchall()
print("Task-related keys found:")
for r in rows:
    print(" ", r[0])

cur.execute("DELETE FROM ItemTable WHERE key LIKE '%task%' OR key LIKE '%Task%'")
print(f"\nDeleted {cur.rowcount} row(s)")
con.commit()
con.close()
print("Done.")
