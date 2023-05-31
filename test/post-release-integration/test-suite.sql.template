


SET custom_extension_repository='net.ednit.duckdb-extensions.s3.us-west-2.amazonaws.com/pytables/${RELEASE_SHA}/python${PYTHON_VERSION}';
INSTALL pytables;
LOAD pytables;


-- Note we don't actually assert the contents of the result. The main thing is to
-- ensure we can exercise DuckDB -> UDF Extension -> Python -> Remote System and back
SELECT *
FROM pytable('ducktables.github:repos_for', 'markroddy',
     columns = {'repo': 'VARCHAR', 'description': 'VARCHAR', 'language': 'VARCHAR'})
WHERE repo like '%duck%';
