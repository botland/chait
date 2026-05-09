# Multi-Agent Mode (Dynamic & Complete)

## Status: COMPLETE (pushed real files 2026-05-09)

- Dynamic agents only (user-spawned with custom prompts)
- LLM orchestrator for routing + auto-spawn
- pthreads + message queue (safe multithreading)
- Fully integrated in main.c ( --multiagent flag)
- Backward compatible with single-agent multiloop

Usage:
spawn-agent --name architect --prompt "..."

Live on GitHub: https://github.com/botland/chait/tree/multiagent/src/multiagent
