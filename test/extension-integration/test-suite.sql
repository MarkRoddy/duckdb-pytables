
load pytables;

-- Note we don't actually assert the contents of the result. The main thing is to
-- ensure we can exercise DuckDB -> UDF Extension -> Python -> Remote System and back
SELECT *
FROM pytable('ducktables.github:repos_for', 'markroddy',
     columns = {'repo': 'VARCHAR', 'description': 'VARCHAR', 'language': 'VARCHAR'})
WHERE repo like '%duck%';
