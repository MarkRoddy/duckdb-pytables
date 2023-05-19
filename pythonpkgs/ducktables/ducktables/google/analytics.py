
from googleapiclient.discovery import build
from google.oauth2 import service_account
from ducktables.google import _key_path

def metrics(view_id, start_date, end_date, dimensions = None, metrics = None, key_file_path = None):
    """
    SQL Usage:
    SELECT * FROM pytable('google_:analytics', '<view-id>', '<start-date>', '<end-date>',
      columns = {
        -- Dimensions
        'date': 'VARCHAR', 'hour': 'VARCHAR', 'pagePath': 'VARCHAR', 'source': 'VARCHAR', 'medium': 'VARCHAR',
        -- Metrics
        'sessions': 'INT', 'users': 'INT', 'pageviews': 'INT', 'avgSessionDuration': 'FLOAT', 'bounceRate': 'FLOAT'
        });

    # Note that this function lets you specify a list of dimensions and/or metrics. If you use non-default
    # values for these you will need to update the 'columns' struct from the example above.
    """
    key_file_path = _key_path(key_file_path)
    # Load credentials from JSON file
    credentials = service_account.Credentials.from_service_account_file(key_file_path)
    
    # Build the Google Analytics Reporting API client
    analytics = build('analyticsreporting', 'v4', credentials=credentials)

    if not metrics:
        metrics = ['ga:sessions', 'ga:users', 'ga:pageviews', 'ga:avgSessionDuration', 'ga:bounceRate']
    elif isinstance(metrics, str):
        metrics = [metrics]
    if not dimensions:
        dimensions = ['ga:date', 'ga:hour', 'ga:pagePath', 'ga:source', 'ga:medium']
    elif isinstance(dimensions, str):
        dimensions = [dimensions]
    
    # Retrieve GA data using the API
    response = analytics.reports().batchGet(
        body={
            'reportRequests': [{
                'viewId': view_id,
                'dateRanges': [{'startDate': start_date, 'endDate': end_date}],
                # 'metrics': [{'expression': 'ga:sessions'}, {'expression': 'ga:users'}],
                'metrics': [ {'expression': m} for m in metrics],
                # 'dimensions': [{'name': 'ga:date'}]
                'dimensions': [{'name': d} for d in dimensions],
            }]
        }
    ).execute()
    
    # Extract data from the API response
    report = response['reports'][0]
    for row in report['data']['rows']:
        # All values are returned as strings because sure why not?
        # So we iterate over each returned value and use a heuristic
        # of if there's a '.' char its a float, otherwise its an int.
        values = row['metrics'][0]['values']
        coerced = []
        for v in values:
            if '.' in v:
                coerced.append(float(v))
            else:
                coerced.append(int(v))
        yield row['dimensions'] + coerced
