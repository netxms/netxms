@Echo use as: git-commit "comments you want to add"
pause
git add .
git commit -a -m %1
git push
