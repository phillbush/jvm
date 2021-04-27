public class Vector1 {
	public static void main(String[] args) {
		int[] arrayOfInt = new int[10];
		double[] arrayOfDouble = { 2.0D, 3.0D, -5.0D };
		long[] arrayOfLong = { -5L, 3L, 6426246L, -433242L };
		float[] arrayOfFloat = { 2.0F, 3.0F, -5.0F };
		byte[] arrayOfByte = { -2, 4, 0 };
		char[] arrayOfChar = { 'a', '0', ')' };
		short[] arrayOfShort = { 15, 1000, -2 };
		byte b;
		for (b = 0; b < 10; b++)
			arrayOfInt[b] = b; 
		arrayOfInt[0] = arrayOfInt[0] + 100000;
		for (b = 0; b < 10; b++)
			System.out.println(arrayOfInt[b]); 
		System.out.println(arrayOfInt.length);
		System.out.println();
		for (b = 0; b < 3; b++)
			System.out.println(arrayOfDouble[b]); 
		System.out.println(arrayOfDouble.length);
		System.out.println();
		for (b = 0; b < 4; b++)
			System.out.println(arrayOfLong[b]); 
		System.out.println(arrayOfLong.length);
		System.out.println();
		for (b = 0; b < 3; b++)
			System.out.println(arrayOfFloat[b]); 
		System.out.println(arrayOfFloat.length);
		System.out.println();
		for (b = 0; b < 3; b++)
			System.out.println(arrayOfByte[b]); 
		System.out.println(arrayOfByte.length);
		System.out.println();
		for (b = 0; b < 3; b++)
			System.out.println(arrayOfChar[b]); 
		System.out.println(arrayOfChar.length);
		System.out.println();
		for (b = 0; b < 3; b++)
			System.out.println(arrayOfShort[b]); 
		System.out.println(arrayOfShort.length);
		System.out.println();
	}
}
