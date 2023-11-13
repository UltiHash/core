import random
import string


def random_string(letters=string.ascii_lowercase, length=8):
   return ''.join(random.choice(letters) for i in range(length))
