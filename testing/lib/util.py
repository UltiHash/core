import random
import string


def get_random_name(letters=string.ascii_lowercase, length=8):
   return ''.join(random.choice(letters) for i in range(length))
