import os
import json
import requests

# Çevre değişkenlerinden DeepSeek API anahtarını al
DEEPSEEK_API_KEY = os.getenv("DEEPSEEK_API_KEY")

def analyze_comment_with_deepseek(comment, api_key):
    """Bir yorumu ve yazarını DeepSeek API ile analiz eder."""
    if not api_key:
        return {"error": "DeepSeek API anahtarı bulunamadı."}

    url = "https://api.deepseek.com/chat/completions"
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json"
    }

    # DeepSeek (ve OpenAI) API'lerinde 'system' ve 'user' rolleri ayrımı daha iyi sonuç verir
    system_prompt = """
    Sen bir GitHub depo moderatörüsün. Benim projem 'Aegis OS', sıfırdan C diliyle yazılmış, 
    donanıma doğrudan erişen (bare-metal) bir mikroçekirdek işletim sistemidir. 

    Sana verilen kullanıcı yorumunu analiz et. Bu yorumun bir insan tarafından mı yoksa 
    bir bot/spam aracı tarafından mı yazıldığını belirle. Cevabını kesinlikle JSON formatında ver.

    Değerlendirme Kriterleri:
    1. Yorum "AI agent skill", "Claude Code", "Firmware Analysis Tool" gibi genel teknoloji terimleri içeriyorsa bottur.
    2. Yorum koddaki bir dosyaya (örn: `kernel.c`), bir fonksiyona veya mimariye spesifik atıf yapıyorsa insandır.

    Lütfen çıktıyı yalnızca aşağıdaki JSON formatında ver:
    {
      "is_bot": true_veya_false,
      "confidence_percent": 0_ile_100_arasi_sayi,
      "reason": "Kararının kısa gerekçesi",
      "action": "Spam veya Guvenli"
    }
    """

    user_content = f"""
    Analiz Edilecek Veri:
    - Kullanıcı Adı: {comment['user']['login']}
    - Yorum Metni: "{comment['body']}"
    """

    # DeepSeek API Payload'u
    payload = {
        "model": "deepseek-chat",
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_content}
        ],
        "response_format": {"type": "json_object"}, # DeepSeek'i sadece JSON dönmeye zorlar
        "temperature": 0.1 # Daha net ve tutarlı kararlar alması için yaratıcılığı kısıyoruz
    }

    try:
        response = requests.post(url, headers=headers, json=payload)
        response.raise_for_status() # HTTP 200 harici bir kod dönerse hata fırlat
        
        response_data = response.json()
        content = response_data['choices'][0]['message']['content']
        
        return json.loads(content)
    except Exception as e:
        return {"error": f"DeepSeek API hatası: {e}"}

def main():
    print("AegisOS Bot Analiz Ajanı (DeepSeek) Tetiklendi!")
    
    event_path = os.getenv("GITHUB_EVENT_PATH")
    
    if not event_path or not os.path.exists(event_path):
        print("Hata: GITHUB_EVENT_PATH bulunamadı. Bu script GitHub Actions ortamında çalışmalıdır.")
        return

    with open(event_path, "r") as f:
        event_data = json.load(f)

    comment = event_data.get("comment")
    action = event_data.get("action")

    if action == "created" and comment:
        username = comment['user']['login']
        print(f"\nYeni yorum yakalandı! Yazan: {username}")
        
        if username == "github-actions[bot]":
            print("Bu yorum kendi botumuz tarafından yapıldı, analiz atlanıyor.")
            return

        analysis = analyze_comment_with_deepseek(comment, DEEPSEEK_API_KEY)
        
        print("\n--- ANALİZ SONUCU ---")
        if "error" in analysis:
            print(f"Hata: {analysis['error']}")
        else:
            print(f"Bot mu?: {'EVET' if analysis.get('is_bot') else 'HAYIR'}")
            print(f"Eminlik: %{analysis.get('confidence_percent')}")
            print(f"Gerekçe: {analysis.get('reason')}")
            print(f"Aksiyon Önerisi: {analysis.get('action')}")
    else:
        print("Tetikleyici bir 'yeni yorum' (comment created) olayı değil.")

if __name__ == "__main__":
    main()
