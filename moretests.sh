echo hello    
echo hello vashisht > a.txt
echo hello saransh > b.txt
cat a.txt b.txt > output.txt
echo bye > c.txt
cat c.txt > output.txt
cat a.txt b.txt c.txt > output.txt
grep hello output.txt