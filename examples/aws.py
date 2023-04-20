
import boto3


def ec2_instances():
    """
    SQL Usage:
    SELECT * FROM python_table('aws', 'ec2_instances',
      {
        'instance_id': 'VARCHAR',
        'instance_type': 'VARCHAR',
        'state': 'VARCHAR',
        'key_name': 'VARCHAR',
        'platform': 'VARCHAR',
        'architecture': 'VARCHAR',
        'vpc_id': 'VARCHAR',
        'subnet_id': 'VARCHAR',
        'public_dns': 'VARCHAR',
        'public_ip': 'VARCHAR',
        'private_dns': 'VARCHAR',
        'private_ip': 'VARCHAR'
        }, []);
    """
    def response_to_rows(response):
        for resv in response['Reservations']:
            resv_id = resv['ReservationId']
            instances = resv['Instances']
            for i in instances:
                yield (
                    i['InstanceId'],
                    i['InstanceType'],
                    i['State']['Name'],
                    i['KeyName'],
                    # i['LaunchTime'].strftime('%m/%d/%Y'),
                    i.get('Platform'),
                    i['Architecture'],
                    i['VpcId'],
                    i['SubnetId'],
                    i['PublicDnsName'],
                    i['PublicIpAddress'],
                    i['PrivateDnsName'],
                    i['PrivateIpAddress'],
                    # i['HibernationOptions']['Configured'],
                    )
    client = boto3.client('ec2')
    response = client.describe_instances()
    yield from response_to_rows(response)
    token = response.get('NextToken')
    while token:
        response = client.describe_instances(NextToken = token)
        yield from response_to_rows(response)
        token = response.get('NextToken')
        
        

def s3_buckets():
    """
    SQL Usage:
    SELECT * FROM python_table('aws', 's3_buckets', {'name': 'VARCHAR', 'creation_date': 'VARCHAR'}, []);
    """
    client = boto3.client('s3')
    response = client.list_buckets()
    for bucket in response['Buckets']:
        yield (bucket['Name'], bucket['CreationDate'].strftime('%m/%d/%Y'))
    

def s3_objects(bucket, prefix = None):
    """
    SQL Usage:
    SELECT * FROM python_table('aws', 's3_objects',
      { 'key': 'VARCHAR', 'last_modified': 'VARCHAR', 'size': 'INT', 'storage_class': 'VARCHAR'},
      ['bucket-name', 'foo/bar/prefix']);

    Note that the final prefix argument is optional, and you don't need to specify it in
    your SQL query. To list all objects:
    SELECT * FROM python_table('aws', 's3_objects',
      { 'key': 'VARCHAR', 'last_modified': 'VARCHAR', 'size': 'INT', 'storage_class': 'VARCHAR'},
      ['bucket-name']);
    """
    def to_row(obj):
        return (obj['Key'],
                obj['LastModified'].strftime('%m/%d/%Y'),
                obj['Size'],
                obj['StorageClass'])

    # Note that the list_objects_v2 doesn't let you specify a meaningless
    # Prefix value. So we can't just pass in a None. We have to check if
    # a value was specified, and only use it as an argument if one exists.
    kwargs = {
        'Bucket': bucket
        }
    if prefix:
        kwargs['Prefix']=prefix

        
    client = boto3.client('s3')
    response = client.list_objects_v2(**kwargs)
    for obj in response.get('Contents', []):
        yield to_row(obj)

    while response['IsTruncated']:
        # Continue to make additional requests, this happens because each
        # list objects request is capped at 1000 responses.
        kwargs['ContinuationToken'] = response['NextContinuationToken']
        response = client.list_objects_v2(**kwargs)
        for obj in response['Contents']:
            yield to_row(obj)
        
