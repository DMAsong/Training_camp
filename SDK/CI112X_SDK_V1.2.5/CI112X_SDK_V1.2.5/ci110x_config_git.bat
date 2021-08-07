git config --local filter.gitignore.clean "sed 's/value=\\\"192.168.[0-9]*.[0-9]*/value=\\\"192.168.2.xxx/'"
git config --local filter.gitignore.smudge cat


@pause