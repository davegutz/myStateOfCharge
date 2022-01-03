"""Test class inheritance"""


class Mother:

    def __init__(self, mother_x_=1):
        self.x = mother_x_
        self.y = -1

    def flop(self, z_):
        print('Mother::flop, z=', z_)
        raise NotImplementedError


class Father:

    def __init__(self, father_x_=2):
        self.x = father_x_
        self.y = -2

    def fee(self, b):
        print('Father.fee: b=', b)

    def fleas(self, b):
        flop = self.flop(12)
        print('Child.flop=', flop)
        print('Father.fleas: b=', b)

    def flop(self, z_):
        print('Father::flop, z_=', z_)
        return z_

    def foo(self, z_):
        raise NotImplementedError


class Child(Mother, Father):

    def __init__(self, x_=-1, z_=-2):
        Father.__init__(self, father_x_=7)
        Father(x_+5)
        self.x = x_
        self.z = z_

    def fee(self, a):
        print('Child.fee: a=', a)

    def flop(self, z_):
        print('Child::flop, z_=', z_)
        return z_

    def foo(self, z_):
        self.z = z_
        return self.y, self.z


child = Child(z_=1)
child.fleas(98)
print('Child.foo(0) = ', child.foo(8))
