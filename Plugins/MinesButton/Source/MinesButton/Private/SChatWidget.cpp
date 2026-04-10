#include "SChatWidget.h"

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

void SChatWidget::Construct(const FArguments& InArgs)
{
	AIRequest = NewObject<UMinesweeperAIRequest>();
	AIRequest->OnResponseReceived.BindRaw(this, &SChatWidget::HandleAIResponse);

	SAssignNew(GridBox, SVerticalBox);

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString("Ask AI for a Minesweeper Grid"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(InputTextBox, SEditableTextBox)
						.HintText(FText::FromString("Enter your request..."))
						.OnTextCommitted(this, &SChatWidget::OnChatSubmitted)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(ResponseText, STextBlock).Text(FText::FromString("AI Response will appear here"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(PlayButton, SButton)
						.Text(FText::FromString("Play"))
						.OnClicked(this, &SChatWidget::GenerateMinesweeperBoard)
						.IsEnabled(false)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					GridBox.ToSharedRef()
				]
		];
}

void SChatWidget::OnChatSubmitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		FString UserInput = Text.ToString();
		AIRequest->SendAIRequest(UserInput);
	}
}

void SChatWidget::HandleAIResponse(FString AIResponse)
{
	// MinesweeperAIRequest already parsed the JSON — we receive either plain text or "Error: ..."
	if (AIResponse.StartsWith(TEXT("Error")))
	{
		UE_LOG(LogTemp, Error, TEXT("AI Error: %s"), *AIResponse);
		ResponseText->SetText(FText::FromString(AIResponse));
		return;
	}

	// Skip the first line (AI preamble like "Here is a 5x5 grid:") and use the rest as the grid
	int32 GridStartIndex = AIResponse.Find(TEXT("\n"));
	LastGeneratedGrid = (GridStartIndex != INDEX_NONE)
		? AIResponse.Mid(GridStartIndex).TrimStartAndEnd()
		: AIResponse;

	// Strip common AI filler lines that aren't part of the grid
	LastGeneratedGrid = LastGeneratedGrid.Replace(TEXT("In this grid, \"X\" represents the bomb."), TEXT(""), ESearchCase::IgnoreCase);
	LastGeneratedGrid = LastGeneratedGrid.Replace(TEXT("The other cells are empty."), TEXT(""), ESearchCase::IgnoreCase);
	LastGeneratedGrid = LastGeneratedGrid.TrimStartAndEnd();

	ResponseText->SetText(FText::FromString(LastGeneratedGrid));
	PlayButton->SetEnabled(true);
}

FReply SChatWidget::GenerateMinesweeperBoard()
{
	UE_LOG(LogTemp, Log, TEXT("Generating Minesweeper Board: \n%s"), *LastGeneratedGrid);
	

	// Convert AI text into a 2D Minesweeper grid, Split AI response into Rows
	TArray<FString> Rows;
	LastGeneratedGrid.ParseIntoArray(Rows, TEXT("\n"), true);

	// Clear old grid (if any)
	GridBox->ClearChildren();

	for (const FString& Row : Rows)
	{
		if (Row.IsEmpty()) continue;

		TSharedPtr<SHorizontalBox> RowBox;// = SNew(SHorizontalBox);
		SAssignNew(RowBox, SHorizontalBox);

		TArray<FString> Cells;
		Row.ParseIntoArray(Cells, TEXT(" "), true);// Split by space (ignores multiple spaces)

		for (const FString& Cell : Cells)
		{

			FString DisplayText = (Cell == "X") ? "X" : Cell; // Bomb 

			RowBox->AddSlot()
				.AutoWidth()
				[
					SNew(SButton)
						.Text(FText::FromString(DisplayText))
				];
		}

		GridBox->AddSlot()
			.AutoHeight()
			[
				RowBox.ToSharedRef()
			];
	}

	return FReply::Handled();
}
