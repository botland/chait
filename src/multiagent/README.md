# Multi-Agent Exploration Branch

## Overview
This branch explores true multi-agent mode for chait:
- 4 specialized agents (Architect, Coder, Tester, Debugger)
- Each runs its own `AgentContext` in a pthread
- Inter-agent communication via lock-protected message queue
- Orchestrator coordinates hand-offs
- Backward compatible with single-agent multiloop

## Architecture
- Single-threaded core preserved
- Optional `--multiagent` flag spawns role threads
- MessageQueue uses pthread_mutex + cond for safe IPC
- No shared globals except the queue (tiny critical section)

## Next steps
- Wire role-specific tool subsets
- Add hand-off logic in orchestrator
- Test end-to-end refactor scenario

Branch created from multiloop at fb474bda443ae368e2ea6b4c259154e43515de66
