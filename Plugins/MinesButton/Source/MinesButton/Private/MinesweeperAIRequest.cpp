#include "MinesweeperAIRequest.h"

void UMinesweeperAIRequest::SendAIRequest(FString UserQuery)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) return;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL("https://api.mistral.ai/v1/chat/completions");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("Authorization", "Bearer xy4B2AuPalvRb4NpSyCUDzd8z8wvXqjJ");

	FString RequestBody = FString::Printf(TEXT(R"({"model": "mistral-small", "messages": [
            {"role": "user", "content": "%s"}
        ], "max_tokens": 100})"), *UserQuery);

	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindUObject(this, &UMinesweeperAIRequest::OnAIResponse);
	Request->ProcessRequest();
}

void UMinesweeperAIRequest::OnAIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response.IsValid())
	{
		FString AIResponse = Response->GetContentAsString();
		OnResponseReceived.ExecuteIfBound(AIResponse);
	}
	else
	{
		OnResponseReceived.ExecuteIfBound("Failed to retrieve AI response.");
	}
}
