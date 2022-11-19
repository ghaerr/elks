int fail = 0;

int main(void)
{
	test_gmtime();
	test_mktime();

	return !!fail;
}
