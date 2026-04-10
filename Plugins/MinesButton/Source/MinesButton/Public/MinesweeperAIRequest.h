#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonUtilities.h"
#include "MinesweeperAIRequest.generated.h"

DECLARE_DELEGATE_OneParam(FOnMinesweeperResponseReceived, FString);

/**
 * Sends requests to an AI provider and extracts the response text.
 * Supports: gemini, openai, claude
 * Configure in Config/Secrets.ini — see Config/Secrets.ini.example
 */
UCLASS()
class MINESBUTTON_API UMinesweeperAIRequest : public UObject
{
	GENERATED_BODY()

public:
	/** Fires with plain text on success, or "Error: ..." on failure. */
	FOnMinesweeperResponseReceived OnResponseReceived;

	/** Reads provider/key/model from Secrets.ini and dispatches the HTTP request. */
	void SendAIRequest(FString UserQuery);

private:
	/** Stored between SendAIRequest and OnAIResponse so the callback knows which schema to parse. */
	FString CurrentProvider;

	void OnAIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
