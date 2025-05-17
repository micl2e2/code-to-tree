I have following hello world programs in 7 different languages:

C:
```
#include <stdio.h>

int main() {
  printf ("Hello!\n");
  return 0;
}
```

C++:
```
#include <iostream>

int main() {
  std::cout << "Hello!" << std::endl;
  return 0;
}
```

Rust:
```
fn main() {
  println!("Hello!");
}
```

Go:
```
package main

import "fmt"

func main() {
    fmt.Println("Hello!")
}
```

Ruby:
```
puts "Hello!"
```

Python:
```
print(R"""Hello!""")
```

Java:
```
public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Hello!");
    }
}
```

Can you generate tree-like, colorful structures for their AST, and put each of them in one SVG? I suggest you embed those SVGs into one HTML, so that the layout could be easily managed. And don't forget node's text should be clearly displayed.
