//1:
    if(nums.size() == 0)
        return = 0;
    int len;
    for (int i=0; i < nums.size(); i++)
    {
        if (nums[i] != nums[len])
        {
            len++;
            nums[len] = nums[i];
        }
    }
    len += 1;
    return len;

//2:
    if (prices.size() == 0)
            return 0;
    int max = 0;
    int tmp = 0;
    for (int i = 1; i < prices.size(); i++)
    {
        tmp = prices[i] - prices[i-1];
        if (tmp > 0)
            max += tmp;
    }
    return max;

//3:
    
