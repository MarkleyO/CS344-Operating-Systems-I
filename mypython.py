import random
import string
import io
import sys

def randomFilename(length=10):
	letters = string.ascii_lowercase
	return ''.join(random.choice(letters) for i in range (length))

for i in range(0, 3):
	f = open("file" + str(i), "w+")
	text = randomFilename()
	f.write(text)
	f.write("\n")
	print(text)
	f.close()

a,b = random.randint(1,43), random.randint(1,43)
print(a, b, a*b, sep="\n")
