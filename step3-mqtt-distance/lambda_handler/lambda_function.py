import json
import urllib.request

def lambda_handler(event, context):

    '''
    TODO:
    センサーの情報から家電操作を制御する。
    送られてきたセンサー情報はevent変数（辞書型）に入っているので、取り出して制御する
    '''
    
    url = 'https://api-google.gh.auhome.au.com/smarthome/execute/commands'
    data = "[\"on\"]"
    headers = {
        "Accept" : 'application/json',
        "Authorization" : 'Bearer {アクセストークン}',
        "Content-Type" : 'application/json;charset=UTF-8' 
    }
    
    req = urllib.request.Request(url, data.encode(), headers)
    with urllib.request.urlopen(req) as res:
        body = res.read()
    
    return event

