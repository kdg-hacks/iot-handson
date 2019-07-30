from requests_oauthlib import OAuth1Session #OAuthのライブラリの読み込み
from datetime import datetime, timedelta, timezone

CK = #"Twitter API Key"
CS = #"Twitter API secret key"
AT = #"Twitter account Access token"
ATS = #"Twitter account Access token secret"
twitter = OAuth1Session(CK, CS, AT, ATS) #認証処理

url = "https://api.twitter.com/1.1/statuses/update.json" #タイムライン取得エンドポイント

def lambda_handler(event, context):
    temperature = event["temperature"]
    humidity = event["humidity"]
    datestr = datetime.now(timezone(timedelta(hours=+9), 'JST')).strftime("%Y-%m-%d-%H:%M:%S")
    
    tweet = "{0}\n現在の室温は{1}度です\n現在の湿度は{2}%です".format(datestr,temperature,humidity)
    
    params = {"status" : tweet}
    
    res = twitter.post(url, params = params)
    
    if res.status_code == 200: #正常通信出来た場合
        print("Seccess")
    else: #正常通信出来なかった場合
        print("Failed: %d" % res.status_code)

