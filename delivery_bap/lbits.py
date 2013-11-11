
class LBits(long):
    """
    Class acting like long, but showing bits for grins.
    2003-12-17 alpha version by Bengt Richter
    """
    class __metaclass__(type):
        """Wrap long methods to return LBits objects"""
        def __new__(cls, name, bases, cdict):
            exclude = [
                '__class__', '__cmp__', '__coerce__', '__delattr__',
                '__divmod__', '__doc__', '__float__', '__getattribute__', '__getnewargs__',
                '__hash__', '__hex__', '__init__', '__int__', '__invert__', '__long__',
                '__new__', '__nonzero__', '__oct__', '__pos__',
                '__rdivmod__', '__reduce__', '__reduce_ex__', '__repr__',
                '__rtruediv__', '__setattr__', '__str__', '__truediv__']
            for k, v in vars(long).items():
                if k in exclude: continue
                exec """\
def %s(self,*args,**kw):
    return self.__class__(long.%s(self,*args,**kw))
""" % (k, k) in cdict
            return type.__new__(cls, name, bases, cdict)
    def __new__(cls, v):
        """Handle additional string literal format with 'b' suffix and sign as leading bit"""
        if isinstance(v, str) and v[-1:].lower()=='b':
            if v[0]=='1':
                return long.__new__(cls, long(v[1:-1] or '0', 2)-(1L<<(len(v)-2)))
            else:
                return long.__new__(cls, v[1:-1] or '0', 2)
        return long.__new__(cls, v)
    def __repr__(self):
        """Show self in round-trip literal format, with leading bit as sign bit and 'b' suffix"""
        val = long(self)
        absval = abs(val)
        s=[chr(((val>>b)&1)+48) for b in xrange(len(hex(val))*4) if not b or absval>>(b-1)]
        s.reverse()
        return ''.join(s+['b'])
    def __iter__(self):
        """Return little-endian iterator through bits, including sign bit last"""
        return self.bitgen()
    def bitgen(self):
        """Little-endian bit sequence generator"""
        bits = long(self)
        sign = bits<0
        while bits>0 or sign and bits!=-1L:
            yield int(bits&1)
            bits >>= 1
        yield int(sign)
