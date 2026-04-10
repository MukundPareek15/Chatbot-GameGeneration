# Minesweeper AI Chat Plugin — Unreal Engine 5

An Unreal Engine 5 Editor plugin that opens a Slate-based chat window. Type a request, an AI model generates a Minesweeper grid, and clicking **Play** renders it as an interactive button grid inside the editor.

Supports **Gemini, OpenAI, and Claude** — bring your own API key.

---

## Features

- AI-generated Minesweeper grids from natural-language prompts
- Works with any of three AI providers (Gemini, OpenAI, Claude)
- API key stored in a local gitignored file — never touches source control
- Slate-only UI (no UMG dependency)
- Rate-limit and API error messages surfaced directly in the chat window

---

## Prerequisites

| Requirement | Version / Notes |
|---|---|
| Unreal Engine | 5.4 or 5.5 (5.3 is deprecated as of UE 5.6) |
| C++ toolchain | Visual Studio 2022 with **Desktop development with C++** and **Game development with C++** workloads |
| IDE | Visual Studio 2022 **or** JetBrains Rider 2024+ |
| AI API key | One of: Google AI Studio (Gemini), OpenAI Platform, or Anthropic Console |

> **Rider users:** Go to **Settings → Build, Execution, Deployment → Toolset and Build** and set MSBuild to Visual Studio's binary (`...\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`), not Rider's bundled one. Without this, the C++ props import will fail.

---

## First-Time Setup

### Step 1 — Clone the repository

```bash
git clone https://github.com/<your-username>/Chatbot-GameGeneration.git
cd Chatbot-GameGeneration
```

### Step 2 — Configure your AI provider

Copy the example secrets file and fill it in:

```bash
cp Config/Secrets.ini.example Config/Secrets.ini
```

Open `Config/Secrets.ini` and set your provider and key:

```ini
[AI]
Provider=gemini        ; gemini | openai | claude
APIKey=YOUR_KEY_HERE
Model=                 ; leave blank to use the default for your provider
```

`Secrets.ini` is listed in `.gitignore` and will never be committed.

**Where to get an API key:**

| Provider | Free tier | Key URL |
|---|---|---|
| Gemini | Yes (daily quota) | https://aistudio.google.com/app/apikey |
| OpenAI | No (pay-as-you-go) | https://platform.openai.com/api-keys |
| Claude | Limited free via Console | https://console.anthropic.com/settings/keys |

### Step 3 — Generate project files

**Right-click** `JoinTask.uproject` in Explorer → **Generate Rider Project Files** (or **Generate Visual Studio project files**).

Or from a terminal:

```bash
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\GenerateProjectFiles.bat" \
  -project="D:\Dev\Unreal Games\Chatbot-GameGeneration\JoinTask.uproject" -game
```

### Step 4 — Build

**In Rider:** Open `JoinTask.sln` → Build → Build Solution (`Ctrl+F9`)

**In Visual Studio:** Open `JoinTask.sln` → Build → Build Solution (`Ctrl+Shift+B`)

**In the Unreal Editor:** Open `JoinTask.uproject` → click **Compile** in the toolbar.

### Step 5 — Open and use

1. Open `JoinTask.uproject` in Unreal Engine 5.
2. Look for the **MinesButton** button in the editor toolbar.
3. Click it — a chat window opens.
4. Type a request, for example:

   ```
   Generate a 5x5 Minesweeper grid with 3 bombs
   ```

5. Press **Enter**. The AI response appears.
6. Click **Play** to render the interactive grid.

---

## File Structure

```
Chatbot-GameGeneration/
│
├── Config/
│   ├── Secrets.ini           # Your API key — gitignored, never committed
│   ├── Secrets.ini.example   # Template to copy from
│   └── Default*.ini          # Standard UE config files
│
├── Source/
│   ├── JoinTask.Target.cs        # Game target (sets IncludeOrderVersion = Latest)
│   ├── JoinTaskEditor.Target.cs  # Editor target
│   └── JoinTask/
│       ├── JoinTask.Build.cs
│       ├── JoinTask.h / .cpp
│
└── Plugins/
    └── MinesButton/
        ├── MinesButton.uplugin
        ├── Resources/
        │   └── Icon128.png
        └── Source/MinesButton/
            ├── Public/
            │   ├── MinesweeperAIRequest.h  # HTTP + JSON abstraction per provider
            │   ├── SChatWidget.h           # Slate widget declaration
            │   ├── WindowHUD.h
            │   ├── MinesButton.h
            │   ├── MinesButtonCommands.h
            │   └── MinesButtonStyle.h
            └── Private/
                ├── MinesweeperAIRequest.cpp  # Builds requests & parses responses
                │                             # for Gemini, OpenAI, and Claude
                ├── SChatWidget.cpp           # Chat UI, grid rendering
                ├── WindowHUD.cpp
                ├── MinesButton.cpp           # Plugin entry point, toolbar button
                ├── MinesButtonCommands.cpp
                └── MinesButtonStyle.cpp
```

### Key components

**`MinesweeperAIRequest`** (`Public/` + `Private/`)
Handles all network and JSON logic. Reads `Config/Secrets.ini` to determine which provider to use, builds the correct request body and headers, sends the HTTP request, parses the provider-specific response JSON, and fires a delegate with plain text (or an `"Error: ..."` string on failure). `SChatWidget` never touches JSON.

**`SChatWidget`** (`Public/` + `Private/`)
The Slate chat widget. Owns a text input, a response label, a Play button, and a dynamic grid box. On receiving a plain-text AI response it strips preamble lines, stores the grid string, and renders it as `SButton` cells when Play is clicked.

**`MinesButton`** (`MinesButton.cpp`)
Plugin module entry point. Registers the toolbar button and the `FMinesButtonCommands` action that opens the `WindowHUD`-backed chat window.

---

## AI Provider Reference

Default models are applied automatically when `Model=` is left blank in `Secrets.ini`.

| Provider | Default model | Notes |
|---|---|---|
| `gemini` | `gemini-2.0-flash` | Free tier available; daily quota resets at midnight Pacific |
| `openai` | `gpt-4o-mini` | Pay-as-you-go; cheapest option for OpenAI |
| `claude` | `claude-haiku-4-5-20251001` | Fastest Claude model; limited free tier via Console |

Other models you can set in `Secrets.ini`:

```ini
# Gemini
Model=gemini-1.5-flash-8b   # separate free-tier quota from 2.0-flash

# OpenAI
Model=gpt-4o

# Claude
Model=claude-sonnet-4-6
```

---

## Troubleshooting

**Plugin button does not appear in the toolbar**
Enable the plugin under **Edit → Plugins → Other → MinesButton**, then restart the editor.

**"Error: Add your API key to Config/Secrets.ini"**
`Secrets.ini` is missing or `APIKey=` is still set to the placeholder. Copy `Secrets.ini.example` and fill in your key.

**"Error 429: You exceeded your current quota"**
The free-tier daily limit is exhausted. Either wait for the midnight reset, switch to a different model (e.g. `gemini-1.5-flash-8b` has a separate quota), or enable billing on your API account.

**"Error: Set Provider= in Config/Secrets.ini"**
The `Provider=` line is missing or blank. Set it to `gemini`, `openai`, or `claude`.

**Build error: `Microsoft.Cpp.Default.props was not found`**
Rider's bundled MSBuild doesn't include VC++ toolchain files. Fix: **Settings → Build, Execution, Deployment → Toolset and Build** → point MSBuild to the Visual Studio 2022 installation, not Rider's bundled version. See the Prerequisites section above.

**Build warning: `EngineIncludeOrderVersion.Unreal5_3 is obsolete`**
Already fixed in this repo — both Target.cs files use `EngineIncludeOrderVersion.Latest`.

**AI response looks garbled / grid doesn't render**
Try a more explicit prompt: `Generate a 5x5 Minesweeper grid with exactly 4 bombs. Use X for bombs and . for empty cells. One cell per space, one row per line.`

---

## Contributing

Pull requests are welcome. For larger changes, open an issue first to discuss the approach.

---

## License

Educational use. See `LICENSE.txt` for details.

---

## Acknowledgments

- Epic Games — Unreal Engine 5
- Google — Gemini API
- OpenAI — GPT models
- Anthropic — Claude models
