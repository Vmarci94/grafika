# grafika

## Komplex számok algebrája

Komplex szám: z = x + y * i = r*e<sup>(i * φ)</sup>, ahol i<sup>2</sup> = -1
* 2D pont, Összeadás = Eltolás
* Szorzás = Forgatva nyújtás

```c++
struct Complex {
   float x, y;
   Complex(float x0, float y0) { x = x0, y = y0; }
   Complex operator+(Complex r) { return Complex(x + r.x, y + r.y); }
   Complex operator-(Complex r) { return Complex(x - r.x, y - r.y); }
   Complex operator*(Complex r) { 
	return Complex(x * r.x - y * r.y, x * r.y + y * r.x); 
   }	
   Complex operator/(Complex r) { 
	float l = r.x * r.x + r.y * r.y;
	return (*this) * Complex(r.x / l, -r.y / l); 
   }
};

Complex Polar(float r, float phi) { 
    return Complex(r * cos(phi), r * sin(phi)); 
}
```

## Feladat

A p pontot az (1,-1) pivot pont körül nyújtsuk 2-szeresére és forgassuk el t-vel, majd toljuk el a (2, 3) vektorral és végül nyújtsuk az origó körül 0.8-szorosára és forgassuk –t/2-radiánnal.

```c++
Complex p, tp;
Complex pivot(1, -1);
tp = ((p - pivot) * Polar(2,t) + pivot + Complex(2, 3)) * Polar(0.8, -t/2);
```

