# Dynamic Multi-Agent Mode (multiagent branch)

## Philosophy
- **No hardcoded roles** (no architect/coder/tester/debugger enums)
- User spawns any number of agents at runtime with fully custom system prompts
- Each agent runs in its own pthread with isolated AgentContext + ReAct loop

## CLI Commands (inside chait when --multiagent enabled)

spawn-agent --name architect --prompt "You are a senior software architect. Always output Mermaid diagrams first."
spawn-agent --name tester --prompt "You are a ruthless tester. Write tests, run them via PTY, fail fast."

@architect design the web search feature
@tester verify the patch

## Status
- spawn_dynamic_agent() implemented and thread-ready
- send_to_agent() stub with message queue integration path
- Orchestrator ready for PTY input parsing
- 100% backward compatible with single-agent multiloop

Next step: wire `spawn-agent` parser into main PTY loop + pthreads in Makefile.
