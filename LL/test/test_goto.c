void func()
{
	int a;
begin:
	a--;
	if(a > b)
		goto begin;
	else
		goto end;
	b++;
end:
	return;
}
