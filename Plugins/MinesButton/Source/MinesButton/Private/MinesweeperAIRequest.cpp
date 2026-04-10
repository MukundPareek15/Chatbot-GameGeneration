#include "MinesweeperAIRequest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakePostRequest(
	const FString& URL, const FString& Body)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(URL);
	Req->SetVerb("POST");
	Req->SetHeader("Content-Type", "application/json");
	Req->SetContentAsString(Body);
	return Req;
}

static FString SerializeJson(TSharedPtr<FJsonObject> Obj)
{
	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Obj.ToSharedRef(), Writer);
	return Out;
}

// ---------------------------------------------------------------------------
// Request building — one function per provider
// ---------------------------------------------------------------------------

static FString BuildGeminiBody(const FString& Model, const FString& UserQuery)
{
	TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
	TextPart->SetStringField(TEXT("text"), UserQuery);

	TArray<TSharedPtr<FJsonValue>> Parts;
	Parts.Add(MakeShareable(new FJsonValueObject(TextPart)));

	TSharedPtr<FJsonObject> Content = MakeShareable(new FJsonObject);
	Content->SetArrayField(TEXT("parts"), Parts);

	TArray<TSharedPtr<FJsonValue>> Contents;
	Contents.Add(MakeShareable(new FJsonValueObject(Content)));

	TSharedPtr<FJsonObject> GenConfig = MakeShareable(new FJsonObject);
	GenConfig->SetNumberField(TEXT("maxOutputTokens"), 300);

	TSharedPtr<FJsonObject> Body = MakeShareable(new FJsonObject);
	Body->SetArrayField(TEXT("contents"), Contents);
	Body->SetObjectField(TEXT("generationConfig"), GenConfig);
	return SerializeJson(Body);
}

static FString BuildOpenAIBody(const FString& Model, const FString& UserQuery)
{
	TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject);
	Message->SetStringField(TEXT("role"), TEXT("user"));
	Message->SetStringField(TEXT("content"), UserQuery);

	TArray<TSharedPtr<FJsonValue>> Messages;
	Messages.Add(MakeShareable(new FJsonValueObject(Message)));

	TSharedPtr<FJsonObject> Body = MakeShareable(new FJsonObject);
	Body->SetStringField(TEXT("model"), Model);
	Body->SetArrayField(TEXT("messages"), Messages);
	Body->SetNumberField(TEXT("max_tokens"), 300);
	return SerializeJson(Body);
}

static FString BuildClaudeBody(const FString& Model, const FString& UserQuery)
{
	TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject);
	Message->SetStringField(TEXT("role"), TEXT("user"));
	Message->SetStringField(TEXT("content"), UserQuery);

	TArray<TSharedPtr<FJsonValue>> Messages;
	Messages.Add(MakeShareable(new FJsonValueObject(Message)));

	TSharedPtr<FJsonObject> Body = MakeShareable(new FJsonObject);
	Body->SetStringField(TEXT("model"), Model);
	Body->SetArrayField(TEXT("messages"), Messages);
	Body->SetNumberField(TEXT("max_tokens"), 300);
	return SerializeJson(Body);
}

// ---------------------------------------------------------------------------
// Response parsing — extracts just the text, or returns empty string on failure
// ---------------------------------------------------------------------------

static FString ParseGeminiResponse(TSharedPtr<FJsonObject> Json)
{
	const TArray<TSharedPtr<FJsonValue>>* Candidates;
	if (!Json->TryGetArrayField(TEXT("candidates"), Candidates) || Candidates->Num() == 0)
		return FString();

	TSharedPtr<FJsonObject> Content = (*Candidates)[0]->AsObject()->GetObjectField(TEXT("content"));
	const TArray<TSharedPtr<FJsonValue>>* Parts;
	if (!Content.IsValid() || !Content->TryGetArrayField(TEXT("parts"), Parts) || Parts->Num() == 0)
		return FString();

	FString Text;
	(*Parts)[0]->AsObject()->TryGetStringField(TEXT("text"), Text);
	return Text;
}

static FString ParseOpenAIResponse(TSharedPtr<FJsonObject> Json)
{
	const TArray<TSharedPtr<FJsonValue>>* Choices;
	if (!Json->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
		return FString();

	TSharedPtr<FJsonObject> Message = (*Choices)[0]->AsObject()->GetObjectField(TEXT("message"));
	FString Text;
	if (Message.IsValid())
		Message->TryGetStringField(TEXT("content"), Text);
	return Text;
}

static FString ParseClaudeResponse(TSharedPtr<FJsonObject> Json)
{
	const TArray<TSharedPtr<FJsonValue>>* Content;
	if (!Json->TryGetArrayField(TEXT("content"), Content) || Content->Num() == 0)
		return FString();

	FString Text;
	(*Content)[0]->AsObject()->TryGetStringField(TEXT("text"), Text);
	return Text;
}

// ---------------------------------------------------------------------------
// UMinesweeperAIRequest
// ---------------------------------------------------------------------------

void UMinesweeperAIRequest::SendAIRequest(FString UserQuery)
{
	FString SecretsIniPath = FPaths::ProjectConfigDir() / TEXT("Secrets.ini");
	GConfig->LoadFile(SecretsIniPath);

	FString Provider, APIKey, Model;
	GConfig->GetString(TEXT("AI"), TEXT("Provider"), Provider, SecretsIniPath);
	GConfig->GetString(TEXT("AI"), TEXT("APIKey"),   APIKey,   SecretsIniPath);
	GConfig->GetString(TEXT("AI"), TEXT("Model"),    Model,    SecretsIniPath);
	Provider = Provider.ToLower().TrimStartAndEnd();

	if (APIKey.IsEmpty() || APIKey.StartsWith(TEXT("YOUR_")))
	{
		OnResponseReceived.ExecuteIfBound(TEXT("Error: Add your API key to Config/Secrets.ini"));
		return;
	}
	if (Provider.IsEmpty())
	{
		OnResponseReceived.ExecuteIfBound(TEXT("Error: Set Provider=gemini|openai|claude in Config/Secrets.ini"));
		return;
	}

	CurrentProvider = Provider;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

	if (Provider == TEXT("gemini"))
	{
		if (Model.IsEmpty()) Model = TEXT("gemini-2.0-flash");
		Request->SetURL(FString::Printf(
			TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent"), *Model));
		Request->SetHeader("x-goog-api-key", APIKey);
		Request->SetContentAsString(BuildGeminiBody(Model, UserQuery));
	}
	else if (Provider == TEXT("openai"))
	{
		if (Model.IsEmpty()) Model = TEXT("gpt-4o-mini");
		Request->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *APIKey));
		Request->SetContentAsString(BuildOpenAIBody(Model, UserQuery));
	}
	else if (Provider == TEXT("claude"))
	{
		if (Model.IsEmpty()) Model = TEXT("claude-haiku-4-5-20251001");
		Request->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		Request->SetHeader("x-api-key", APIKey);
		Request->SetHeader("anthropic-version", TEXT("2023-06-01"));
		Request->SetContentAsString(BuildClaudeBody(Model, UserQuery));
	}
	else
	{
		OnResponseReceived.ExecuteIfBound(
			FString::Printf(TEXT("Error: Unknown provider '%s'. Use gemini, openai, or claude."), *Provider));
		return;
	}

	Request->OnProcessRequestComplete().BindUObject(this, &UMinesweeperAIRequest::OnAIResponse);
	Request->ProcessRequest();
}

void UMinesweeperAIRequest::OnAIResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnResponseReceived.ExecuteIfBound(TEXT("Error: Network request failed"));
		return;
	}

	FString Raw = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Raw AI Response:\n%s"), *Raw);

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		OnResponseReceived.ExecuteIfBound(TEXT("Error: Failed to parse AI response as JSON"));
		return;
	}

	// All three providers use {"error": {"message": "..."}} for errors
	const TSharedPtr<FJsonObject>* ErrorObj;
	if (Json->TryGetObjectField(TEXT("error"), ErrorObj))
	{
		FString Msg;
		(*ErrorObj)->TryGetStringField(TEXT("message"), Msg);

		// Gemini includes a retryDelay in details[]
		FString RetryHint;
		const TArray<TSharedPtr<FJsonValue>>* Details;
		if ((*ErrorObj)->TryGetArrayField(TEXT("details"), Details))
		{
			for (const TSharedPtr<FJsonValue>& D : *Details)
			{
				FString Delay;
				if (D->AsObject().IsValid() && D->AsObject()->TryGetStringField(TEXT("retryDelay"), Delay))
				{
					RetryHint = FString::Printf(TEXT(" (retry in %s)"), *Delay);
					break;
				}
			}
		}

		int32 Code = 0;
		(*ErrorObj)->TryGetNumberField(TEXT("code"), Code);
		OnResponseReceived.ExecuteIfBound(
			FString::Printf(TEXT("Error %d: %s%s"), Code, *Msg.Left(150), *RetryHint));
		return;
	}

	// Extract provider-specific text
	FString Text;
	if      (CurrentProvider == TEXT("gemini")) Text = ParseGeminiResponse(Json);
	else if (CurrentProvider == TEXT("openai")) Text = ParseOpenAIResponse(Json);
	else if (CurrentProvider == TEXT("claude")) Text = ParseClaudeResponse(Json);

	if (Text.IsEmpty())
	{
		OnResponseReceived.ExecuteIfBound(TEXT("Error: Could not extract text from response"));
		return;
	}

	OnResponseReceived.ExecuteIfBound(Text);
}
