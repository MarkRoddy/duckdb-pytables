import duckdb

def test_python_udf():
    conn = duckdb.connect('');
    conn.execute("SELECT python_udf('Sam') as value;");
    res = conn.fetchall()
    assert(res[0][0] == "Python_udf Sam 🐥");