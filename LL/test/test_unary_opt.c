void fun()
{
	int i;
	a = -1;
	a = -a;
	a = &a;
	a = !a;
	*str = *ch;
	*str = **ch;
	str = ***ch;
	str = **ch;
	str = *ch;
	**str = *ch;
}
