# Git Commit 前缀规范

## 常用前缀

| 前缀        | 全称                     | 含义           | 示例                                                 |
|-----------|------------------------|--------------|----------------------------------------------------|
| feat:     | Feature                | 新功能          | `feat: add user login with OAuth2`                 |
| fix:      | Fix                    | 修复 bug       | `fix: resolve memory leak in WebSocket connection` |
| docs:     | Documentation          | 文档更新         | `docs: update API usage examples in README`        |
| style:    | Style                  | 代码格式化（不影响逻辑） | `style: format code with prettier`                 |
| refactor: | Refactor               | 重构（不改变功能）    | `refactor: extract validation logic into utils`    |
| perf:     | Performance            | 性能优化         | `perf: cache database query results`               |
| test:     | Test                   | 添加或修改测试      | `test: add unit tests for auth module`             |
| chore:    | Chore                  | 构建工具、依赖、配置更新 | `chore: update .gitignore to exclude logs`         |
| build:    | Build                  | 构建系统或外部依赖变更  | `build: upgrade webpack to v5`                     |
| ci:       | Continuous Integration | CI 配置修改      | `ci: add GitHub Actions workflow for testing`      |
| revert:   | Revert                 | 回滚提交         | `revert: revert "feat: add dark mode"`             |

## 进阶用法 (Conventional Commits)

### 带 scope

指定影响范围，格式：`<type>(<scope>): <description>`

```
feat(auth): implement JWT refresh token
fix(api): handle null response in user endpoint
docs(readme): add installation instructions
```

### Breaking Change

在类型后加 `!` 表示破坏性变更：

```
feat(api)!: change response format from XML to JSON
refactor!: drop support for Node 14
```

### 完整格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

示例：

```
feat(wallet): add multi-chain support

- Support Ethereum, Polygon, and Arbitrum
- Add chain switching UI component
- Implement RPC fallback mechanism

BREAKING CHANGE: wallet.connect() now requires chainId parameter
Closes #123
```

## 参考

- [Conventional Commits](https://www.conventionalcommits.org/)
- [Angular Commit Guidelines](https://github.com/angular/angular/blob/main/CONTRIBUTING.md#commit)