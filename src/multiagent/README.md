# Dynamic Multi-Agent Mode (multiagent branch)

## Philosophy
- **No hardcoded roles** (no architect/coder/tester/debugger enums)
- User spawns any number of agents at runtime with fully custom system prompts
- **New evolution**: The LLM orchestrator can *automatically request* to spawn a new agent if none of the existing ones fit the task.

## CLI Commands (inside chait when --multiagent enabled)

spawn-agent --name architect --prompt "You are a senior software architect..."

@architect design the web search feature

## LLM-Auto-Spawn (new feature)
Orchestrator LLM sees all agents + their prompts.
If no match: it replies `SPAWN:newname:full custom prompt`
→ automatically spawns + routes.

## Status
- spawn_dynamic_agent() implemented
- `llm_route_or_spawn()` added — LLM decides route OR auto-spawn
- send_to_agent() stub ready
- 100% backward compatible

Next: wire parser + pthreads + full inference call.

**Pushed live.**
